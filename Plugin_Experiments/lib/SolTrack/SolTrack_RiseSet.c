/**
 * @brief  Compute rise, transit and set times for the Sun, as well as their azimuths/altitude
 *
 * @param [in]    time                   Struct containing date and time to compute the position for, in UT
 * @param [in]    location               Struct containing the geographic location to compute the position for
 * @param [out]   position               Struct containing the position of the Sun
 * @param [out]   riseSet                Struct containing the Sun's rise, transit and set data
 * @param [in]    rsAlt                  Altitude to return rise/set data for (radians; 0. is actual rise/set).  rsAlt>pi/2: compute transit only
 * @param [in]    useDegrees             Use degrees for input and output angular variables, rather than radians
 * @param [in]    useNorthEqualsZero     Use the definition where azimuth=0 denotes north, rather than south
 *
 * Example Usage:
 * @code SolTrack(time, location, &position, 0.0); @endcode
 *
 * @note
 * - if rsAlt == 0.0, actual rise and set times are computed
 * - if rsAlt != 0.0, the routine calculates when alt = rsAlt is reached
 * - returns times, rise/set azimuth and transit altitude in the struct position
 *
 * @see
 * - subroutine riset() in riset.f90 from libTheSky (libthesky.sf.net) for more info
 */

#include "SolTrack.h"

void SolTrack_RiseSet(struct Time time,  struct Location location, struct Position *position, struct RiseSet *riseSet, double rsAlt, int useDegrees, int useNorthEqualsZero) {
  int evi,iter;
  double tmdy[3],  cosH0,h0,th0,dTmdy,accur,  ha,alt,azalt[3];
  
  alt=0.0;  ha=0.0; h0=0.0;  tmdy[0]=0.0; tmdy[1]=0.0; tmdy[2]=0.0;  azalt[0]=0.0; azalt[1]=0.0; azalt[2]=0.0;
  
  int computeRefrEquatorial = 1;  // Compure refraction-corrected equatorial coordinates (Hour angle, declination): 0-no, 1-yes
  int computeDistance = 0;        // Compute the distance to the Sun in AU: 0-no, 1-yes
  
  double rsa = -0.8333/R2D;               // Standard altitude for the Sun in radians
  if(fabs(rsAlt) > 1.e-9) rsa = rsAlt;    // Use a user-specified altitude
  
  // If the used uses degrees, convert the geographic location to radians:
  struct Location llocation = location;  // Create local variable
  if(useDegrees) {
    llocation.longitude /= R2D;
    llocation.latitude  /= R2D;
  }
  
  // Set date and time to midnight UT for the desired day:
  struct Time rsTime;
  rsTime.year   = time.year;
  rsTime.month  = time.month;
  rsTime.day    = time.day;
  
  rsTime.hour   = 0;
  rsTime.minute = 0;
  rsTime.second = 0.0;
  
  SolTrack(rsTime, llocation, position, 0, useNorthEqualsZero, computeRefrEquatorial, computeDistance);  // useDegrees = 0: NEVER use degrees internally!
  double agst0 = position->agst;  // AGST for midnight
  
  int evMax = 2;                  // Compute transit, rise and set times by default (0-2)
  cosH0 = (sin(rsa)-sin(llocation.latitude)*sin(position->declination)) / (cos(llocation.latitude)*cos(position->declination));
  if(fabs(cosH0) > 1.0) {         // Body never rises/sets
    evMax = 0;                    // Compute transit time and altitude only
  } else {
    h0 = rev(2.0*acos(cosH0))/2.0;
  }
  
  
  tmdy[0] = rev(position->rightAscension - llocation.longitude - position->agst);  // Transit time in radians; lon0 > 0 for E
  if(evMax > 0) {
    tmdy[1] = rev(tmdy[0] - h0);   // Rise time in radians
    tmdy[2] = rev(tmdy[0] + h0);   // Set time in radians
  }
  
  for(evi=0; evi<=evMax; evi++) {  // Transit, rise, set
    iter = 0;
    accur = 1.0e-5;                // Accuracy;  1e-5 rad ~ 0.14s. Don't make this smaller than 1e-16
    
    dTmdy = INFINITY;
    while(fabs(dTmdy) > accur) {
      th0 = agst0 + 1.002737909350795*tmdy[evi];  // Solar day in sidereal days in 2000
      
      rsTime.second = tmdy[evi]*R2H*3600.0;       // Radians -> seconds - w.r.t. midnight (h=0,m=0)
      SolTrack(rsTime, llocation, position, 0, useNorthEqualsZero, computeRefrEquatorial, computeDistance);  // useDegrees = 0: NEVER use degrees internally!
      
      ha  = rev2(th0 + llocation.longitude - position->rightAscension);        // Hour angle
      alt = asin(sin(llocation.latitude)*sin(position->declination) + 
		 cos(llocation.latitude)*cos(position->declination)*cos(ha));  // Altitude
      
      // Correction to transit/rise/set times:
      if(evi==0) {           // Transit
	dTmdy = -rev2(ha);
      } else {               // Rise/set
	dTmdy = (alt-rsa)/(cos(position->declination)*cos(llocation.latitude)*sin(ha));
      }
      tmdy[evi] = tmdy[evi] + dTmdy;
      
      // Print debug output to stdOut:
      //printf(" %4i %2i %2i  %2i %2i %9.3lf    ", rsTime.year,rsTime.month,rsTime.day, rsTime.hour,rsTime.minute,rsTime.second);
      //printf(" %3i %4i   %9.3lf %9.3lf %9.3lf \n", evi,iter, tmdy[evi]*24,fabs(dTmdy)*24,accur*24);
      
      iter += 1;
      if(iter > 30) break;  // while loop doesn't converge
    }  // while(fabs(dTmdy) > accur)
    
    
    if(iter > 30) {  // Convergence failed
      printf("\n  *** WARNING:  riset():  Riset failed to converge: %i %9.3lf  ***\n", evi,rsAlt);
      tmdy[evi]  = -INFINITY;
      azalt[evi] = -INFINITY;
    } else {               // Result converged, store it
      if(evi == 0) {
	azalt[evi] = alt;                                                                         // Transit altitude
      } else {
	azalt[evi] = atan2( sin(ha), ( cos(ha) * sin(llocation.latitude)  -
				       tan(position->declination) * cos(llocation.latitude) ) );   // Rise,set hour angle -> azimuth
      }
    }
    
    if(tmdy[evi] < 0.0 && fabs(rsAlt) < 1.e-9) {
      tmdy[evi]     = -INFINITY;
      azalt[evi] = -INFINITY;
    }
    
  }  // for-loop evi
  
  
  // Set north to zero radians for azimuth if desired:
  if(useNorthEqualsZero) {
    azalt[1] = rev(azalt[1] + PI);  // Add PI and fold between 0 and 2pi
    azalt[2] = rev(azalt[2] + PI);  // Add PI and fold between 0 and 2pi
  }
  
  // Convert resulting angles to degrees if desired:
  if(useDegrees) {
    azalt[0]  *=  R2D;   // Transit altitude
    azalt[1]  *=  R2D;   // Rise azimuth
    azalt[2]  *=  R2D;   // Set azimuth
  }
  
  // Store results:
  riseSet->transitTime      =  tmdy[0]*R2H;  // Transit time - radians -> hours
  riseSet->riseTime         =  tmdy[1]*R2H;  // Rise time - radians -> hours
  riseSet->setTime          =  tmdy[2]*R2H;  // Set time - radians -> hours
  
  riseSet->transitAltitude  =  azalt[0];     // Transit altitude
  riseSet->riseAzimuth      =  azalt[1];     // Rise azimuth
  riseSet->setAzimuth       =  azalt[2];     // Set azimuth
}



/**
 * @brief  Fold an angle to take a value between 0 and 2pi
 *
 * @param [in] angle  Angle to fold (radians)
 */

double rev(double angle) {
  return fmod(angle + MPI, TWO_PI);    // Add 1e6*PI and use the modulo function to ensure 0 < angle < 2pi
}

/**
 * @brief  Fold an angle to take a value between -pi and pi
 *
 * @param [in] angle  Angle to fold (radians)
 */

double rev2(double angle) {
  return fmod(angle + PI + MPI, TWO_PI) - PI;    // Add 1e6*PI and use the modulo function to ensure -pi < angle < pi
}


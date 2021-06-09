/*
  SolTrack: a simple, free, fast and accurate C routine to compute the position of the Sun
  
  
  Copyright (c) 2014-2017  Marc van der Sluys, Paul van Kan and Jurgen Reintjes,
  Lectorate of Sustainable Energy, HAN University of applied sciences, Arnhem, The Netherlands
   
  This file is part of the SolTrack package, see: http://soltrack.sourceforge.net
  SolTrack is derived from libTheSky (http://libthesky.sourceforge.net) under the terms of the GPL v.3
  
  This is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
  
  This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public License along with this code.  If not, see 
  <http://www.gnu.org/licenses/>.
*/


#include "SolTrack.h"


/**
 * @brief   Main function to compute the position of the Sun
 *
 * @param [in]    time                   Struct containing date and time to compute the position for, in UT
 * @param [inout] location               Struct containing the geographic location to compute the position for
 * @param [out]   position               Struct containing the position of the Sun in horizontal (and equatorial if desired) coordinates
 * 
 * @param [in]    useDegrees             Use degrees for input and output angular variables, rather than radians
 * @param [in]    useNorthEqualsZero     Use the definition where azimuth=0 denotes north, rather than south
 * @param [in]    computeRefrEquatorial  Compute refraction correction for equatorial coordinates: 0-no, 1-yes
 * @param [in]    computeDistance        Compute distance to the Sun (in AU): 0-no, 1-yes
 *
 * Example Usage:
 * @code SolTrack(time, location, &position, 1); @endcode
 */

void SolTrack(struct Time time, struct Location location, struct Position *position,  int useDegrees, int useNorthEqualsZero, int computeRefrEquatorial, int computeDistance) {
  
  // If the used uses degrees, convert the geographic location to radians:
  struct Location llocation = location;  // Create local variable
  if(useDegrees) {
    llocation.longitude /= R2D;
    llocation.latitude  /= R2D;
  }
  
  
  // Compute these once and reuse:
  llocation.sinLat = sin(llocation.latitude);
  llocation.cosLat = sqrt(1.0 - llocation.sinLat * llocation.sinLat);  // Cosine of a latitude is always positive or zero
  
  
  
  // Compute the Julian Day from the date and time:
  position->julianDay = computeJulianDay(time.year, time.month, time.day,  time.hour, time.minute, time.second);
  
  
  // Derived expressions of time:
  position->tJD  = position->julianDay - 2451545.0;                    // Time in Julian days since 2000.0
  position->tJC  = position->tJD/36525.0;                              // Time in Julian centuries since 2000.0
  position->tJC2 = position->tJC*position->tJC;                        // T^2
  
  
  // Compute the ecliptic longitude of the Sun and the obliquity of the ecliptic:
  computeLongitude(computeDistance, position);
  
  // Convert ecliptic coordinates to geocentric equatorial coordinates:
  convertEclipticToEquatorial(position->longitude, position->cosObliquity,  &position->rightAscension, &position->declination);
  
  // Convert equatorial coordinates to horizontal coordinates, correcting for parallax and refraction:
  convertEquatorialToHorizontal(llocation, position);
  
  
  // Convert the corrected horizontal coordinates back to equatorial coordinates:
  if(computeRefrEquatorial) convertHorizontalToEquatorial(llocation.sinLat, llocation.cosLat, position->azimuthRefract, \
							  position->altitudeRefract, &position->hourAngleRefract, &position->declinationRefract);
  
  // Use the North=0 convention for azimuth and hour angle (default: South = 0) if desired:
  if(useNorthEqualsZero) setNorthToZero(&position->azimuthRefract, &position->hourAngleRefract, computeRefrEquatorial);
  
  // If the user wants degrees, convert final results from radians to degrees:
  if(useDegrees) convertRadiansToDegrees(&position->longitude,  &position->rightAscension, &position->declination,  \
					 &position->altitude, &position->azimuthRefract, &position->altitudeRefract,  \
					 &position->hourAngleRefract, &position->declinationRefract, computeRefrEquatorial);
}





/**
 * @brief  Compute the Julian Day from the date and time
 * 
 * Gregorian calendar only (>~1582).
 *
 * @param [in]  year    Year of date
 * @param [in]  month   Month of date
 * @param [in]  day     Day of date
 * @param [in]  hour    Hour of time
 * @param [in]  minute  Minute of time
 * @param [in]  second  Second of time
 *
 * @retval JulianDay    Julian day for the given date and time
 */

double computeJulianDay(int year, int month, int day,  int hour, int minute, double second) {
  if(month <= 2) {
    year -= 1;
    month += 12;
  }
  
  int tmp1 = (int)floor(year/100.0);
  int tmp2 = 2 - tmp1 + (int)floor(tmp1/4.0);
  
  double dDay = day + hour/24.0 + minute/1440.0 + second/86400.0;
  double JD = floor(365.250*(year+4716)) + floor(30.60010*(month+1)) + dDay + tmp2 - 1524.5;
  
  return JD;
}



/**
 * @brief  Compute the ecliptic longitude of the Sun for a given Julian Day
 * 
 * Also computes the obliquity of the ecliptic and nutation.
 *
 * @param [in]     computeDistance  Compute distance to the Sun (in AU): 0-no, 1-yes
 * @param [inout]  position  Position of the Sun
 */

void computeLongitude(int computeDistance, struct Position *position) {
  
  double l0 = 4.895063168 + 628.331966786 * position->tJC  +  5.291838e-6  * position->tJC2;  // Mean longitude
  double m  = 6.240060141 + 628.301955152 * position->tJC  -  2.682571e-6  * position->tJC2;  // Mean anomaly
  
  
  double c = (3.34161088e-2 - 8.40725e-5* position->tJC - 2.443e-7*position->tJC2)*sin(m) + (3.489437e-4 - 1.76278e-6*position->tJC)*sin(2*m);   // Sun's equation of the centre
  double odot = l0 + c;                                                                       // True longitude
  
  
  // Nutation, aberration:
  double omg  = 2.1824390725 - 33.7570464271 * position->tJC  + 3.622256e-5 * position->tJC2;                               // Lon. of Moon's mean ascending node
  double dpsi = -8.338601e-5*sin(omg);                                                                                      // Nutation in longitude
  double dist = 1.0000010178;                                                                                               // Mean distance to the Sun in AU
  if(computeDistance) {
    double ecc = 0.016708634 - 0.000042037   * position->tJC  -  0.0000001267 * position->tJC2;                             // Eccentricity of the Earth's orbit
    double nu = m + c;                                                                                                      // True anomaly
    dist = dist*(1.0 - ecc*ecc)/(1.0 + ecc*cos(nu));                                                                        // Geocentric distance of the Sun in AU
  }
  double aber = -9.93087e-5/dist;                                                                                           // Aberration
  
  // Obliquity of the ecliptic and nutation - do this here, since we've already computed many of the ingredients:
  double eps0 = 0.409092804222 - 2.26965525e-4*position->tJC - 2.86e-9*position->tJC2;                                      // Mean obliquity of the ecliptic
  double deps = 4.4615e-5*cos(omg);                                                                                         // Nutation in obliquity
  
  // Save position parameters:
  position->longitude = odot + aber + dpsi;                                                                                 // Apparent geocentric longitude, referred to the true equinox of date
  while(position->longitude > TWO_PI) position->longitude -= TWO_PI;
  
  position->distance = dist;                                                                                                // Distance (AU)
  
  position->obliquity   = eps0 + deps;                                                                                      // True obliquity of the ecliptic
  position->cosObliquity = cos(position->obliquity);                                                                        // Need the cosine later on
  position->nutationLon = dpsi;                                                                                             // Nutation in longitude
}


/**
 * @brief  Convert ecliptic coordinates to equatorial coordinates
 * 
 * This function assumes that the ecliptic latitude = 0.
 *
 * @param [in] longitude  Ecliptic longitude of the Sun (rad)
 * @param [in] cosObliquity  Cosine of the obliquity of the ecliptic
 * 
 * @param [out] rightAscension  Right ascension of the Sun (rad)
 * @param [out] declination     Declination of the Sun (rad)
 */

void convertEclipticToEquatorial(double longitude, double cosObliquity,  double *rightAscension, double *declination) {
  double sinLon = sin(longitude);
  double sinObl = sqrt(1.0-cosObliquity*cosObliquity);               // Sine of the obliquity of the ecliptic will be positive in the forseeable future

  *rightAscension   = STatan2(cosObliquity*sinLon, cos(longitude));  // 0 <= azimuth < 2pi
  *declination      = asin(sinObl*sinLon);
}



/**
 * @brief  Convert equatorial to horizontal coordinates
 * 
 * Also corrects for parallax and atmospheric refraction.
 *
 * @param [in]    location  Geographic location of the observer (rad)
 * @param [inout] position  Position of the Sun (rad)
 */

void convertEquatorialToHorizontal(struct Location location, struct Position *position) {
  double gmst    = 4.89496121 + 6.300388098985*position->tJD + 6.77e-6*position->tJC2;  // Greenwich mean sidereal time
  position->agst = gmst + position->nutationLon * position->cosObliquity;               // Correction for equation of the equinoxes -> apparent Greenwich sidereal time
  
  
  double sinAlt=0.0;
  // Azimuth does not need to be corrected for parallax or refraction, hence store the result in the 'azimuthRefract' variable directly:
  eq2horiz(location.sinLat,location.cosLat, location.longitude,  position->rightAscension, position->declination, position->agst,  &position->azimuthRefract, &sinAlt);
  
  double alt = asin( sinAlt );                                     // Altitude of the Sun above the horizon (rad)
  double cosAlt = sqrt(1.0 - sinAlt * sinAlt);                     // Cosine of the altitude is always positive or zero
  
  // Correct for parallax:
  alt -= 4.2635e-5 * cosAlt;                                       // Horizontal parallax = 8.794" = 4.2635e-5 rad
  position->altitude = alt;
  
  // Correct for atmospheric refraction:
  double dalt = 2.967e-4 / tan(alt + 3.1376e-3/(alt + 8.92e-2));   // Refraction correction in altitude
  dalt *= location.pressure/101.0 * 283.0/location.temperature;
  alt += dalt;
  // to do: add pressure/temperature dependence
  position->altitudeRefract = alt;
}



/**
 * @brief  Convert equatorial coordinates to horizontal coordinates
 * 
 * @param [in]  sinLat          Sine of the geographic latitude of the observer
 * @param [in]  cosLat          Cosine of the geographic latitude of the observer
 * @param [in]  longitude       Geographic longitude of the observer (rad)
 * @param [in]  rightAscension  Right ascension of the Sun (rad)
 * @param [in]  declination     Declination of the Sun (rad)
 * @param [in]  agst            Apparent Greenwich sidereal time (Greenwich mean sidereal time, corrected for the equation of the equinoxes)
 * 
 * @param [out] azimuth         Azimuth ("wind direction") of the Sun (rad; 0=South)
 * @param [out] sinAlt          Sine of the altitude of the Sun above the horizon
 */

void eq2horiz(double sinLat, double cosLat, double longitude,  double rightAscension, double declination, double agst,   double *azimuth, double *sinAlt) {
  
  double ha  = agst + longitude - rightAscension;                      // Local Hour Angle
  
  // Some preparation, saves ~29%:
  double sinHa  = sin(ha);
  double cosHa  = cos(ha);
  
  double sinDec = sin(declination);
  double cosDec = sqrt(1.0 - sinDec * sinDec);                         // Cosine of a declination is always positive or zero
  double tanDec = sinDec/cosDec;
  
  *azimuth = STatan2( sinHa,  cosHa  * sinLat - tanDec * cosLat );     // 0 <= azimuth < 2pi
  *sinAlt = sinLat * sinDec + cosLat * cosDec * cosHa;                 // Sine of the altitude above the horizon
}



/**
 * @brief  Convert (refraction-corrected) horizontal coordinates to equatorial coordinates
 * 
 * @param [in]  sinLat       Sine of the geographic latitude of the observer
 * @param [in]  cosLat       Cosine of the geographic latitude of the observer
 * @param [in]  azimuth      Azimuth ("wind direction") of the Sun (rad; 0=South)
 * @param [in]  altitude     Altitude of the Sun above the horizon (rad)
 * @param [out] hourAngle    Hour angle of the Sun (rad; 0=South)
 * @param [out] declination  Declination of the Sun (rad)
 */

void convertHorizontalToEquatorial(double sinLat, double cosLat, double azimuth, double altitude, double *hourAngle, double *declination) {
  
  // Multiply used variables:
  double cosAz  = cos(azimuth);
  double sinAz  = sin(azimuth);                                            // For symmetry
  
  double sinAlt = sin(altitude);
  double cosAlt = sqrt(1.0 - sinAlt * sinAlt);                             // Cosine of an altitude is always positive or zero
  double tanAlt = sinAlt/cosAlt;
  
  *hourAngle   = STatan2( sinAz ,  cosAz  * sinLat + tanAlt * cosLat );    // Local Hour Angle:  0 <= hourAngle < 2pi
  *declination = asin(  sinLat * sinAlt  -  cosLat * cosAlt * cosAz  );    // Declination
}



/**
 * @brief  Convert the South=0 convention to North=0 convention for azimuth and hour angle
 * 
 * South=0 is the default in celestial astronomy.  This makes the angles compatible with the compass/wind directions.
 * 
 * @param [inout] azimuth                Azimuth ("wind direction") of the Sun (rad)
 * @param [inout] hourAngle              Hour angle of the Sun (rad)
 * @param [in]    computeRefrEquatorial  Compute refraction correction for equatorial coordinates
 */

void setNorthToZero(double *azimuth, double *hourAngle, int computeRefrEquatorial) {
  *azimuth = *azimuth + PI;                            // Add PI to set North=0
  if(*azimuth > TWO_PI) *azimuth -= TWO_PI;            // Ensure 0 <= azimuth < 2pi

  if(computeRefrEquatorial) {
    *hourAngle = *hourAngle + PI;                      // Add PI to set North=0
    if(*hourAngle > TWO_PI) *hourAngle -= TWO_PI;      // Ensure 0 <= hour angle < 2pi
  }
}



/**
 * @brief  Convert final results from radians to degrees
 * 
 * Not touching intermediate results.
 * 
 * @param [inout] azimuth                Azimuth ("wind direction") of the Sun (rad->deg)
 * @param [inout] altitude               Altitude of the Sun above the horizon (rad->deg)
 * @param [inout] hourAngle              Hour angle of the Sun (rad->deg)
 * @param [inout] declination            Declination of the Sun (rad->deg)
 * @param [in]    computeRefrEquatorial  Compute refraction correction for equatorial coordinates
 */

void convertRadiansToDegrees(double *longitude,  double *rightAscension, double *declination,  double *altitude, double *azimuthRefract, double *altitudeRefract,  \
			     double *hourAngle, double *declinationRefract, int computeRefrEquatorial) {
  *longitude *= R2D;
  *rightAscension *= R2D;
  *declination *= R2D;
  
  *altitude *= R2D;
  *azimuthRefract *= R2D;
  *altitudeRefract *= R2D;
  if(computeRefrEquatorial) {
    *hourAngle *= R2D;
    *declinationRefract *= R2D;
  }
}



/**
 * @brief  My version of the atan2() function - ~39% faster than built-in (in terms of number of instructions)
 * 
 * @param [in] y   Numerator of the fraction to compute the arctangent for
 * @param [in] x   Denominator of the fraction to compute the arctangent for
 * 
 * 
 * atan2(y,x) = atan(y/x), where the result will be put in the correct quadrant
 * 
 * @see  https://en.wikipedia.org/wiki/Atan2#Definition_and_computation
 */

double STatan2(double y, double x) {
  
  if(x > 0.0) {
    
    return atan(y/x);
    
  } else if(x < 0.0) {
    
    if(y >= 0.0) {
      return atan(y/x) + PI;
    } else {  // y < 0
      return atan(y/x) - PI;
    }
    
  } else {  // x == 0
    
    if(y > 0.0) {
      return PI/2.0;
    } else if(y < 0.0) {
      return -PI/2.0;
    } else {  // y == 0
      return 0.0;
    }
    
  }
  
}



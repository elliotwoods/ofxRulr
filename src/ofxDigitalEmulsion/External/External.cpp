#include "External.h"
#include "Manager.h"

#include "./Item/KinectV2.h"
#include "./Procedure/Calibrate/CameraFromKinectV2.h"
#include "./Procedure/Calibrate/ProjectorFromKinectV2.h"

namespace ofxDigitalEmulsion {
	namespace External {
		//----------
		void registerExternals() {
			declareNode<Item::KinectV2>();
			declareNode<Procedure::Calibrate::CameraFromKinectV2>();
			declareNode<Procedure::Calibrate::ProjectorFromKinectV2>();
		}
	}
}
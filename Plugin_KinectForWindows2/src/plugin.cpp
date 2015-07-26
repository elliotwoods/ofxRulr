#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Item/KinectV2.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/CameraFromKinectV2.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ProjectorFromKinectV2.h"

OFXPLUGIN_INIT_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_REGISTER(ofxRulr::Nodes::Item::KinectV2);
	OFXPLUGIN_REGISTER(ofxRulr::Nodes::Procedure::Calibrate::CameraFromKinectV2);
	OFXPLUGIN_REGISTER(ofxRulr::Nodes::Procedure::Calibrate::ProjectorFromKinectV2);
OFXPLUGIN_INIT_END
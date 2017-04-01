#include "pch_Plugin_MoCap.h"

#include "ofxPlugin.h"

#include "ofxRulr/Nodes/MoCap/FindMarkerCentroids.h"
#include "ofxRulr/Nodes/MoCap/PreviewCentroids.h"
#include "ofxRulr/Nodes/MoCap/Body.h"
#include "ofxRulr/Nodes/MoCap/MatchMarkers.h"
#include "ofxRulr/Nodes/MoCap/PreviewMatchedMarkers.h"
#include "ofxRulr/Nodes/MoCap/UpdateTracking.h"
#include "ofxRulr/Nodes/MoCap/OSCRelay.h"
#include "ofxRulr/Nodes/MoCap/StereoSolvePnP.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::FindMarkerCentroids);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::PreviewCentroids);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::Body);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::MatchMarkers);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::PreviewMatchedMarkers);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::UpdateTracking);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::OSCRelay);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MoCap::StereoSolvePnP);
OFXPLUGIN_PLUGIN_MODULES_END
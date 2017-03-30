#include "pch_Plugin_ArUco.h"

#include "ofxPlugin.h"

#include "ofxRulr/Nodes/ArUco/Detector.h"
#include "ofxRulr/Nodes/ArUco/Dictionary.h"
#include "ofxRulr/Nodes/ArUco/TrackMarkers.h"
#include "ofxRulr/Nodes/ArUco/MarkerMap.h"
#include "ofxRulr/Nodes/ArUco/FitRoomPlanes.h"
#include "ofxRulr/Nodes/ArUco/ChArUcoBoard.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::Detector);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::Dictionary);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::TrackMarkers);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::MarkerMap);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::FitRoomPlanes);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::ChArUcoBoard);
OFXPLUGIN_PLUGIN_MODULES_END
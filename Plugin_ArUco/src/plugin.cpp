#include "pch_Plugin_ArUco.h"

#include "ofxPlugin.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
{
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::Detector);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::Dictionary);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::FindMarkers);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::MarkerMap);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::MarkerMapPoseTracker);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::AlignMarkerMap);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::ChArUcoBoard);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::ArUco::OSCRelay);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MarkerMap::Markers);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MarkerMap::Calibrate);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MarkerMap::NavigateCamera);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MarkerMap::Transform);

}
OFXPLUGIN_PLUGIN_MODULES_END
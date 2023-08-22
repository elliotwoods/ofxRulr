#include "pch_Plugin_Reworld.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Reworld/Router.h"
#include "ofxRulr/Nodes/Reworld/CameraTest.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::Router);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::CameraTest);
OFXPLUGIN_PLUGIN_MODULES_END
#include "pch_Plugin_DualSense.h"
#include "ofxPlugin.h"
#include "ofxRulr/Nodes/DualSense/Controller.h"


OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::DualSense::Controller);
OFXPLUGIN_PLUGIN_MODULES_END
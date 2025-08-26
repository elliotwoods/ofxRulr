#include "pch_Plugin_Reworld.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Reworld/Router.h"
#include "ofxRulr/Nodes/Reworld/CameraTest.h"
#include "ofxRulr/Nodes/Reworld/Installation.h"
#include "ofxRulr/Nodes/Reworld/ColumnView.h"
#include "ofxRulr/Nodes/Reworld/ModuleView.h"
#include "ofxRulr/Nodes/Reworld/SimulateLightBeams.h"


OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::Router);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::CameraTest);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::Installation);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::ColumnView);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::ModuleView);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::SimulateLightBeams);
OFXPLUGIN_PLUGIN_MODULES_END
#include "pch_Plugin_Reworld.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Reworld/Router.h"
#include "ofxRulr/Nodes/Reworld/CameraTest.h"
#include "ofxRulr/Nodes/Reworld/Installation.h"
#include "ofxRulr/Nodes/Reworld/ColumnView.h"
#include "ofxRulr/Nodes/Reworld/PanelView.h"
#include "ofxRulr/Nodes/Reworld/PortalView.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::Router);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::CameraTest);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::Installation);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::ColumnView);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::PanelView);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Reworld::PortalView);
OFXPLUGIN_PLUGIN_MODULES_END
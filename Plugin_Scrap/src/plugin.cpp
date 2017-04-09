#include "pch_Plugin_Scrap.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/LaserToWorld.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::LaserToWorld);
OFXPLUGIN_PLUGIN_MODULES_END
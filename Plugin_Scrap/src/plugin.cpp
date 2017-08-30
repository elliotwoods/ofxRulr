#include "pch_Plugin_Scrap.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/LaserToWorld.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/FindLine.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ProjectCircle.h"
#include "ofxRulr/Nodes/Item/CircleLaser.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::LaserToWorld);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::FindLine);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ProjectCircle);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Item::CircleLaser);
OFXPLUGIN_PLUGIN_MODULES_END
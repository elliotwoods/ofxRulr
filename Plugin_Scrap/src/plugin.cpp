#include "pch_Plugin_Scrap.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/LaserToWorld.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/FindLine.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ProjectCircle.h"
#include "ofxRulr/Nodes/Item/CircleLaser.h"

#include "ofxRulr/Nodes/AnotherMoon/Lasers.h"
#include "ofxRulr/Nodes/AnotherMoon/RemoteControl.h"
#include "ofxRulr/Nodes/AnotherMoon/Calibrate.h"
#include "ofxRulr/Nodes/AnotherMoon/LaserCaptures.h"
#include "ofxRulr/Nodes/AnotherMoon/BeamCaptures.h"


OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::LaserToWorld);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::FindLine);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ProjectCircle);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Item::CircleLaser);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::Lasers);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::RemoteControl);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::Calibrate);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::LaserCaptures);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::BeamCaptures);
OFXPLUGIN_PLUGIN_MODULES_END
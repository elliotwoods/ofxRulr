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
#include "ofxRulr/Nodes/AnotherMoon/SortFiles.h"
#include "ofxRulr/Nodes/AnotherMoon/Target.h"
#include "ofxRulr/Nodes/AnotherMoon/SynthesiseCaptures.h"
#include "ofxRulr/Nodes/AnotherMoon/Moon.h"
#include "ofxRulr/Nodes/AnotherMoon/DrawMoon.h"
#include "ofxRulr/Nodes/AnotherMoon/ExportPictures.h"


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
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::SortFiles);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::Target);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::SynthesiseCaptures);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::DrawMoon);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::Moon);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::AnotherMoon::ExportPictures);
OFXPLUGIN_PLUGIN_MODULES_END
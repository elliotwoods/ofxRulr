#include "pch_Plugin_BrightnessAssignmentMap.h"

#include "ofxPlugin.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
{
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::BAM::World);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::BAM::Projector);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::BAM::PreviewCoverage);
}
OFXPLUGIN_PLUGIN_MODULES_END
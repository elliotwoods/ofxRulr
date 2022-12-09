#include "pch_Plugin_Caustics.h"

#include "ofxRulr/Nodes/Caustics/Target.h"
#include "ofxRulr/Nodes/Caustics/SimpleSurface.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
{
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Caustics::Target);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Caustics::SimpleSurface);
}
OFXPLUGIN_PLUGIN_MODULES_END
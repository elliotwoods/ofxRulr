#include "pch_Plugin_Dosirak.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Dosirak/OSCReceive.h"


OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Dosirak::OSCReceive);
OFXPLUGIN_PLUGIN_MODULES_END
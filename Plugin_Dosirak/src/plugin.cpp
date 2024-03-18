#include "pch_Plugin_Dosirak.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Dosirak/OSCReceive.h"
#include "ofxRulr/Nodes/Dosirak/Curves.h"
#include "ofxRulr/Nodes/Dosirak/DrawCurves.h"
#include "ofxRulr/Nodes/Dosirak/RecordAndRender.h"


OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Dosirak::OSCReceive);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Dosirak::Curves);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Dosirak::DrawCurves);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Dosirak::RecordAndRender);
OFXPLUGIN_PLUGIN_MODULES_END
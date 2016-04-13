#include "pch_Plugin_Orbbec.h"
#include "ofxPlugin.h"

#include "ofxRulr/Nodes/Item/Orbbec/Device.h"
#include "ofxRulr/Nodes/Item/Orbbec/Color.h"
#include "ofxRulr/Nodes/Item/Orbbec/Infrared.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/Orbbec/ColorRegistration.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/Orbbec/ToPojector.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
{
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Item::Orbbec::Device);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Item::Orbbec::Color);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Item::Orbbec::Infrared);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::Orbbec::ColorRegistration);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::Orbbec::ToProjector);
}
OFXPLUGIN_PLUGIN_MODULES_END
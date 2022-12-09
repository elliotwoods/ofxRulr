#define _HAS_STD_BYTE 0
#include "ofxPlugin.h"

#include "ofxMachineVision/Device/Spinnaker.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxMachineVision::Device::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxMachineVision::Device::Spinnaker);
OFXPLUGIN_PLUGIN_MODULES_END
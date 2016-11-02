#include "pch_Plugin_KinectForWindows2.h"
#include "ofxPlugin.h"

#include "ofxMachineVision/Device/KinectForWindows2RGB.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxMachineVision::Device::Base)
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxMachineVision::Device::KinectForWindows2RGB);
OFXPLUGIN_PLUGIN_MODULES_END;
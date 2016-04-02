#include "pch_MultiTrack.h"


#include "ofxRulr/Nodes/MultiTrack/ChannelGenerator/LocalKinect.h"
#include "ofxRulr/Nodes/MultiTrack/ClientHandler.h"
#include "ofxRulr/Nodes/MultiTrack/Receiver.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ChannelGenerator::LocalKinect);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ClientHandler);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Receiver);
OFXPLUGIN_PLUGIN_MODULES_END
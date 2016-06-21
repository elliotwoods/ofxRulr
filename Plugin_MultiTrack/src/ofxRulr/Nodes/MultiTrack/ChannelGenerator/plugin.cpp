#include "pch_MultiTrack.h"

#include "ofxRulr/Nodes/MultiTrack/ChannelGenerator/LocalKinect.h"
#include "ofxRulr/Nodes/MultiTrack/ClientHandler.h"
// UDP
#include "ofxRulr/Nodes/MultiTrack/Receiver.h"
#include "ofxRulr/Nodes/MultiTrack/Sender.h"
// TCP
#include "ofxRulr/Nodes/MultiTrack/Publisher.h"
#include "ofxRulr/Nodes/MultiTrack/Subscriber.h"
#include "ofxRulr/Nodes/MultiTrack/World.h"
#include "ofxRulr/Nodes/MultiTrack/ImageBox.h"
#include "ofxRulr/Nodes/MultiTrack/Procedure/Calibrate.h"
#include "ofxRulr/Nodes/MultiTrack/Test/FindMarker.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ChannelGenerator::LocalKinect);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ClientHandler);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Sender);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Receiver);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Publisher);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Subscriber);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::World);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ImageBox);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Procedure::Calibrate);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Test::FindMarker);
OFXPLUGIN_PLUGIN_MODULES_END
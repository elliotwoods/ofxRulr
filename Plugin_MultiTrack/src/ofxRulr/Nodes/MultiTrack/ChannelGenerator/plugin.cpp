#include "pch_MultiTrack.h"

#include "ofxRulr/Nodes/MultiTrack/ChannelGenerator/LocalKinect.h"
#include "ofxRulr/Nodes/MultiTrack/ClientHandler.h"
#ifdef OFXMULTITRACK_UDP
#include "ofxRulr/Nodes/MultiTrack/Receiver.h"
#include "ofxRulr/Nodes/MultiTrack/Sender.h"
#endif // OFXMULTITRACK_UDP
#include "ofxRulr/Nodes/MultiTrack/Publisher.h"
#include "ofxRulr/Nodes/MultiTrack/Subscriber.h"
#include "ofxRulr/Nodes/MultiTrack/World.h"
#include "ofxRulr/Nodes/MultiTrack/Procedure/Calibrate.h"
#include "ofxRulr/Nodes/MultiTrack/Test/FindMarker.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ChannelGenerator::LocalKinect);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::ClientHandler);
#ifdef OFXMULTITRACK_UDP
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Sender);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Receiver);
#endif // OFXMULTITRACK_UDP
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Publisher);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Subscriber);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::World);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Procedure::Calibrate);
OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::MultiTrack::Test::FindMarker);
OFXPLUGIN_PLUGIN_MODULES_END
#include "ofxRulr/Nodes/DeclareNodes.h"
#include "ofApp.h"

#if defined(TARGET_WIN32) || defined(RULR_WITH_CANON)
#include "../../../ofxCanon/pairs/ofxMachineVision/Device/Canon.h"
#include "../../../ofxCanon/pairs/ofxMachineVision/Device/CanonLiveView.h"
#include "../../../ofxCanon/pairs/ofxMachineVision/Device/CanonRemote.h"
#include "../../../ofxCanon/pairs/ofxRulr/Nodes/Canon/Control.h"
#include "../../../ofxCanon/pairs/ofxRulr/Nodes/Canon/LiveView.h"
#endif

#ifdef RULR_WITH_KINECT_V2_OSX
#include "../../Plugin_KinectV2OSX/src/ofxRulr/Nodes/Item/KinectV2OSX.h"
#endif
#ifdef RULR_WITH_BLACKMAGIC
#include "../../../ofxBlackmagic2/pairs/ofxMachineVision/Device/DeckLink.h"
#endif

using namespace ofxAssets;

//--------------------------------------------------------------
void ofApp::setup2(){
	ofSetEscapeQuitsApp(false);
	
	auto & world = ofxRulr::Graph::World::X();

	//--
	//Start splash screen
	//--
	//
	ofxAssets::Register::X().addAddon("ofxRulr");
	//splashScreen.init(image("ofxRulr::SplashScreen"));
	//splashScreen.begin(1.0f);
	//
	//--

#ifdef RULR_WITH_KINECT_V2_OSX
    //--
    //Setup OSX nodes
    //--
    //
    RULR_DECLARE_NODE(Nodes::Item::KinectV2OSX);
#endif
#ifdef RULR_WITH_BLACKMAGIC
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::DeckLink>();
    //
    //--
#endif

	//--
	//Setup EDSDK nodes and ofxMachineVision devices
	//--
	//
#if defined(TARGET_WIN32) || defined(RULR_WITH_CANON)
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::Canon>();
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::CanonLiveView>();
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::CanonRemote>();
	RULR_DECLARE_NODE(ofxRulr::Nodes::Canon::Control);
	RULR_DECLARE_NODE(ofxRulr::Nodes::Canon::LiveView);
#endif
	//
	//--
    
	//--
	//Initialize nodes and plugins
	//--
	//
	ofxRulr::Nodes::loadCoreNodes();
	ofxRulr::Nodes::loadPluginNodes();
	//
	//--


	//--
	//Setup the patch
	//--
	//
	auto patchInstance = MAKE(ofxRulr::Graph::Editor::Patch);
	world.add(patchInstance);
	//
	//--


	//--
	//Initialise gui, world and load last patch
	//--
	//
	this->gui.init();
	world.init(this->gui.getController());
	world.loadAll();
	//
	//--



	//--
	//Hide the splash screen, back to business!
	//
	//this->splashScreen.end();
	//
	//--

	ofLogToConsole();
}

//--------------------------------------------------------------
void ofApp::update(){
	ofxRulr::Graph::World::X().update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	if (ofGetFrameNum() == 2) {
		this->setup2();
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

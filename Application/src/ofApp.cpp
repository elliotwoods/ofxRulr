#include "ofxRulr/Nodes/DeclareNodes.h"
#include "ofApp.h"

#include "../../../ofxCanon/pairs/ofxMachineVision/Device/Canon.h"
#include "../../../ofxCanon/pairs/ofxMachineVision/Device/CanonLiveView.h"

#ifdef TARGET_OSX
#include "../Plugin_KinectV2OSX/src/ofxRulr/Nodes/Item/KinectV2OSX.h"
#include "../../../addons/ofxBlackmagic2/pairs/ofxMachineVision/Device/DeckLink.h"
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

#ifdef TARGET_OSX
    //--
    //Setup OSX nodes
    //--
    //
    RULR_DECLARE_NODE(Nodes::Item::KinectV2OSX);
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::DeckLink>();
    //
    //--
#endif

	//--
	//Setup EDSDK nodes
	//--
	//
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::Canon>();
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::CanonLiveView>();
	//
	//--
    
	//--
	//Initialize nodes and plugins
	//--
	//
	ofxRulr::Nodes::loadCoreNodes();
#ifdef TARGET_WIN32
	ofxRulr::Nodes::loadPluginNodes();
#endif
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
}

//--------------------------------------------------------------
void ofApp::update(){
	//ofDrawBitmapString(50, 10, 10);
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

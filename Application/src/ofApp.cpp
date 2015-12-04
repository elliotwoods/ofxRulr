#include "../../../addons/ofxRulr/Nodes/src/ofxRulr/Nodes/DeclareNodes.h"
#include "../../../addons/ofxEdsdk/pairs/ofxMachineVision/CanonDSLRDevice.h"
#include "ofApp.h"

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


	//--
	//Initialise nodes and plugins
	//--
	//
	ofxRulr::Nodes::loadCoreNodes();
	ofxRulr::Nodes::loadPluginNodes();
	ofxMachineVision::Device::FactoryRegister::X().add<ofxMachineVision::Device::CanonDSLRDevice>();
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

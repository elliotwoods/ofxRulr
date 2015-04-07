#include "../../../addons/ofxBlackmagic2/pairs/ofxMachineVision/Device/DeckLink.h"
#include "../../../addons/ofxEdsdk/pairs/ofxMachineVision/Device/CanonDSLRDevice.h"
#include "../../../addons/ofxUeye/src/ofxUeye.h"

#include "ofApp.h"

using namespace ofxAssets;

//--------------------------------------------------------------
void ofApp::setup2(){
	ofSetCircleResolution(120);
	
	AssetRegister.addAddon("ofxDigitalEmulsion");
	splashScreen.init(image("ofxDigitalEmulsion::SplashScreen"));
	splashScreen.begin(0.0f);
	
	ofxMachineVision::registerDevice<ofxMachineVision::Device::CanonDSLRDevice>();
	ofxMachineVision::registerDevice<ofxMachineVision::Device::DeckLink>();
	ofxMachineVision::registerDevice<ofxUeye::Device>();

	ofxDigitalEmulsion::External::registerExternals();

	auto patchInstance = MAKE(ofxDigitalEmulsion::Graph::Editor::Patch);
	this->world.add(patchInstance);

	this->gui.init();
	this->world.init(this->gui.getController());
	this->world.loadAll();

	this->splashScreen.end();
	
}

//--------------------------------------------------------------
void ofApp::update(){
	this->world.update();
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

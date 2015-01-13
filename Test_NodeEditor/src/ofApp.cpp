#include "ofApp.h"

using namespace ofxAssets;

//--------------------------------------------------------------
void ofApp::setup2(){
	ofSetWindowTitle("Digital Emulsion Toolkit v0.2");
	ofSetCircleResolution(120);
	
	AssetRegister.addAddon("ofxDigitalEmulsion");
	splashScreen.init(image("ofxDigitalEmulsion::SplashScreen"));
	splashScreen.begin(0.0f);
	
	auto patchInstance = MAKE(ofxDigitalEmulsion::Graph::Editor::Patch);
	this->world.add(patchInstance);

	this->gui.init();
	this->world.init(this->gui.getController());
	this->world.loadAll();
	
	/*
	auto canonNode = MAKE(Item::Camera);
	auto canonDevice = make_shared<ofxMachineVision::Device::CanonDSLRDevice>();
	canonNode->setDevice(canonDevice);
	canonNode->init();
	patchInstance->addNode(canonNode, ofRectangle(0, 0, 200, 200));
	*/

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

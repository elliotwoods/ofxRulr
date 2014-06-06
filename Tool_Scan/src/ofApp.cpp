#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	auto cameraDevice = MAKE(ofxMachineVision::Device::VideoInputDevice, 1920, 1080, 30.0f);
	auto camera = MAKE(Item::Camera);
	camera->setDevice(cameraDevice);

	auto checkerboard = MAKE(Item::Checkerboard);

	auto calibrator = MAKE(Procedure::Calibrate::CameraIntrinsics);
	calibrator->connect(checkerboard);
	calibrator->connect(camera);
	
	this->world.add(camera);
	this->world.add(checkerboard);
	this->world.add(calibrator);
	
	this->gui.init();
	this->world.setupGui(this->gui.getController());
	
	Panels::Inspector::onClear += [] (ElementGroupPtr inspector) {
		inspector->add(Widgets::LiveValueHistory::make("Application fps [Hz]", [] () {
			return ofGetFrameRate();
		}, true));
	};
}

//--------------------------------------------------------------
void ofApp::update(){
	this->world.update();
}

//--------------------------------------------------------------
void ofApp::draw(){

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

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowShape(1280, 720/2);

	auto cameraDevice = ofxMachineVision::DevicePtr(new ofxMachineVision::Device::VideoInputDevice(1920, 1080, 30.0f));
	auto camera = Item::Camera::make();
	this->world.add(camera);
	camera->setDevice(cameraDevice);

	auto checkerboard = Item::Checkerboard::make();
	this->world.add(checkerboard);
	
	this->gui.init();
	this->cameraPanel = this->gui.add(* camera->getGrabber(), "Camera");
	this->gui.addInspector();

	this->cameraPanel->onMouseReleased += [this, camera] (MouseArguments &) {
		inspect(* camera);
	};

	Panels::Inspector::onClear += [] (ElementGroupPtr inspector) {
		inspector->add(Widgets::LiveValueHistory::make("Application fps [Hz]", [] () {
			return ofGetFrameRate();
		}, true));
	};

	//set the inspector on the camera to begin with
	inspect(* camera);
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

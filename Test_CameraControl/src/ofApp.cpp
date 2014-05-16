#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	auto cameraDevice = DevicePtr(new Device::VideoInputDevice(2304, 1536, 2.0f));
	auto camera = this->world.addCamera();
	camera->setDevice(cameraDevice);

	this->gui.init();
	this->gui.addInspector();
	auto cameraPanel = this->gui.add(* camera->getGrabber(), "Camera");

	cameraPanel->onMouseReleased += [this, camera] (MouseArguments &) {
		inspect(* camera);
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

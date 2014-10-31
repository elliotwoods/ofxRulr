#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowTitle("Camera-Projector correspondences scan");

	auto cameraDevice = MAKE(ofxMachineVision::Device::VideoInputDevice, 1920, 1080, 30.0f);
	auto camera = MAKE(Item::Camera);
	camera->setDevice(cameraDevice);

	auto projector = MAKE(Item::Projector);

	auto scan = MAKE(Procedure::Scan::Graycode);

	scan->connect(camera);
	scan->connect(projector);
	
	this->world.add(camera);
	this->world.add(projector);
	this->world.add(scan);
	
	this->gui.init();
	this->world.init(this->gui.getController());
	this->world.loadAll();
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

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowTitle("Triangulate scan data");

	auto cameraDevice = MAKE(ofxMachineVision::Device::VideoInputDevice, 1920, 1080, 30.0f);
	auto camera = MAKE(Item::Camera);
	camera->setDevice(cameraDevice);

	auto projector = MAKE(Item::Projector);

	auto checkerboard = MAKE(Item::Checkerboard);

	auto cameraCalibrate = MAKE(Procedure::Calibrate::CameraIntrinsics);
	cameraCalibrate->connect(camera);
	cameraCalibrate->connect(checkerboard);
	
	auto projectorCalibrate = MAKE(Procedure::Calibrate::ProjectorIntrinsicsExtrinsics);
	projectorCalibrate->connect(projector);
	projectorCalibrate->connect(camera);
	projectorCalibrate->connect(checkerboard);

	auto graycode = MAKE(Procedure::Scan::Graycode);
	graycode->connect(camera);
	graycode->connect(projector);
	
	auto triangulate = MAKE(Procedure::Triangulate);
	triangulate->connect(camera);
	triangulate->connect(projector);
	triangulate->connect(graycode);
	
	this->world.add(camera);
	this->world.add(projector);
	this->world.add(checkerboard);
	this->world.add(cameraCalibrate);
	this->world.add(projectorCalibrate);
	this->world.add(graycode);
	this->world.add(triangulate);
	
	this->gui.init();
	this->world.setupGui(this->gui.getController());
	this->world.loadAll();

	//start with having the camera selected
	ofxCvGui::inspect(* camera);
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

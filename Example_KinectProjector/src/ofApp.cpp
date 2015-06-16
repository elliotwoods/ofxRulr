#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	kinect.open();
	kinect.initDepthSource();
	kinect.initColorSource();
	kinect.initBodyIndexSource();

	auto result = ofSystemLoadDialog("Select ofxRay::Camera file for this projector");
	if (!result.bSuccess) {
		ofExit();
	}
	ifstream filein(ofToDataPath(result.filePath), ios::binary | ios::in);
	filein >> this->projector;
	filein.close();

	ofSetBackgroundColor(0);
}

//--------------------------------------------------------------
void ofApp::update(){
	this->kinect.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
	this->projector.beginAsCamera();
	this->kinect.drawWorld();
	this->projector.endAsCamera();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

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

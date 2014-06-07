#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetVerticalSync(true);
	ofBackground(0);
	ofHideCursor();
	
	osc.setup("localhost", 4005);
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofxSpinCursor::draw(ofGetMouseX(), ofGetMouseY());
}

//--------------------------------------------------------------
void ofApp::sendWithCursor(string address) {
	auto cursor = ofVec2f(ofGetMouseX(), ofGetMouseY());
	cursor /= ofVec2f(ofGetWidth(), ofGetHeight());
	cursor *= 2.0f;
	cursor -= 1.0f;
	cursor.y = 1.0f - cursor.y;
	
	ofxOscMessage m;
	m.setAddress(address);
	m.addFloatArg(cursor.x);
	m.addFloatArg(cursor.y);
	m.addFloatArg(ofGetWidth());
	m.addFloatArg(ofGetHeight());
	osc.sendMessage(m);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == 'f') {
		ofToggleFullscreen();
	} else {
		string stringKey;
		stringKey.resize(1);
		stringKey[0] = (char) key;
		this->sendWithCursor(stringKey);
	}
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
	this->sendWithCursor("/cursor");
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

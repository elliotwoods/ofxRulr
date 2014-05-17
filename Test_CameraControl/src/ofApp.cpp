#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowShape(1280, 720/2);

	this->showGraph.set("Show graph", true);

	auto cameraDevice = DevicePtr(new Device::VideoInputDevice(1920, 1080, 30.0f));
	auto camera = this->world.addCamera();
	camera->setDevice(cameraDevice);

	this->gui.init();
	this->cameraPanel = this->gui.add(* camera->getGrabber(), "Camera");
	this->gui.addInspector();

	this->cameraPanel->onMouseReleased += [this, camera] (MouseArguments &) {
		inspect(* camera);
		addToInspector(Widgets::Toggle::make(this->showGraph));
	};
	this->cameraPanel->onDrawCropped += [this] (Panels::BaseImage::DrawCroppedArguments & args) {
		if (this->showGraph) {
			ofPushMatrix();
			ofPushStyle();

			ofTranslate(0, args.size.y / 2.0f);
			ofSetColor(100, 255, 100);
			ofLine(0,0, args.size.x, 0);
			
			ofTranslate(0, +128);
			ofScale(1.0f, -1.0f);
			ofSetColor(0, 0, 0);
			ofSetLineWidth(2.0f);
			this->graph.draw();
			ofSetLineWidth(1.0f);
			ofSetColor(255, 100, 100);
			this->graph.draw();

			ofPopStyle();
			ofPopMatrix();
		}
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

	if (this->showGraph) {
		auto grabber = this->world.get<ofxDigitalEmulsion::Device::Camera>()->getGrabber();
		if (grabber->isFrameNew()) {
			auto pixels = grabber->getPixelsRef();
			auto middleRow = pixels.getPixels() + pixels.getWidth() * pixels.getNumChannels() * pixels.getHeight() / 2;

			this->graph.clear();
			this->graph.setMode(OF_PRIMITIVE_LINE_STRIP);
			for(int i=0; i<pixels.getWidth(); i++) {
				this->graph.addVertex(ofVec3f(i, *middleRow, 0));
				middleRow += pixels.getNumChannels();
			}
		}
	}
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

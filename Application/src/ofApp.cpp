#include "../../../addons/ofxRulr/Nodes/src/ofxRulr/Nodes/DeclareNodes.h"

#include "ofApp.h"

using namespace ofxAssets;

//--------------------------------------------------------------
void ofApp::setup2(){
	ofSetEscapeQuitsApp(false);
	
	//--
	//Start splash screen
	//--
	//
	ofxAssets::Register::X().addAddon("ofxRulr");
	splashScreen.init(image("ofxRulr::SplashScreen"));
	splashScreen.begin(1.0f);
	//
	//--
	

	//--
	//Initialise camera drivers
	//--
	//
	// cameras should generally be plugins, so these will come later
	//
	//--

	
	//--
	//Initialise nodes
	//--
	//
	ofxRulr::Nodes::loadCoreNodes();
	ofxRulr::Nodes::loadPluginNodes();
	//
	//--



	//--
	//Setup the patch
	//--
	//
	auto patchInstance = MAKE(ofxRulr::Graph::Editor::Patch);
	this->world.add(patchInstance);
	//
	//--



	//--
	//Initialise the Gui and the World
	//--
	//
	this->gui.init();
	this->world.init(this->gui.getController());
	this->world.loadAll();
	//
	//--



	//--
	//Hide the splash screen, back to business!
	//
	this->splashScreen.end();
	//
	//--
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

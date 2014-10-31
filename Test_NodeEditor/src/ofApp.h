#pragma once

#include "ofMain.h"
#include "ofxCvGui.h"
#include "ofxMachineVision.h"
#include "ofxDigitalEmulsion.h"
#include "ofxSplashScreen.h"
#include "ofxUeye.h"

using namespace ofxMachineVision;
using namespace ofxDigitalEmulsion;
using namespace ofxCvGui;

class ofApp : public ofBaseApp{

	public:
		void setup2();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		ofxCvGui::Builder gui;
		ofxDigitalEmulsion::Graph::World world;
		ofxSplashScreen splashScreen;
};

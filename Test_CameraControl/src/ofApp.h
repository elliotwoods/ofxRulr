#pragma once

#include "ofMain.h"
#include "ofxCvGui.h"
#include "ofxMachineVision.h"
#include "ofxDigitalEmulsion.h"

using namespace ofxMachineVision;
using namespace ofxCvGui;

class ofApp : public ofBaseApp{

	public:
		void setup();
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
		ofxDigitalEmulsion::Device::World world;

		shared_ptr<Panels::BaseImage> cameraPanel;
		ofParameter<bool> showGraph;
		ofMesh graph;
};

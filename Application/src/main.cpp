#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main() {
	ofGLFWWindowSettings windowSettings;
	windowSettings.setGLVersion(2, 0);
	windowSettings.width = 1920;
	windowSettings.height = 1080;
	windowSettings.depthBits = 32;
	auto window = ofCreateWindow(windowSettings);

	ofRunApp(new ofApp());
}

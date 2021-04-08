#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main() {
	ofGLFWWindowSettings windowSettings;
	windowSettings.setGLVersion(RULR_GL_VERSION_MAJOR, RULR_GL_VERSION_MINOR);
	windowSettings.setSize(1920, 1080);
	windowSettings.depthBits = 32;
	auto window = ofCreateWindow(windowSettings);

	ofRunApp(new ofApp());
}

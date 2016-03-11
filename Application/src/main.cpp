#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main() {
	ofGLFWWindowSettings windowSettings;
	windowSettings.setGLVersion(RULR_GL_VERSION_MAJOR, RULR_GL_VERSION_MINOR);
	windowSettings.width = 1920;
	windowSettings.height = 1080;
	auto window = ofCreateWindow(windowSettings);

	ofRunApp(new ofApp());
}

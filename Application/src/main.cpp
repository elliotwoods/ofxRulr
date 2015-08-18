#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofGLFWWindowSettings windowSettings;
	windowSettings.setGLVersion(2, 0); //according to http://www.glfw.org/docs/latest/window.html#window_hints GLFW will attempt to provide the highest supported context version if you specify 1.0
	windowSettings.width = 1920;
	windowSettings.height = 1080;
	windowSettings.windowMode = OF_WINDOW;
	auto window = ofCreateWindow(windowSettings);

	ofRunApp(new ofApp());
}

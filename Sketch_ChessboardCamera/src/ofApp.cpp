#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	camera.open(1);
	camera.setExposure(700);
	camera.setGain(0);
	camera.setFocus(0.0f);
	
	sizeWidth.set("Board Size Width", 9, 1, 20);
	sizeHeight.set("Board Size Height", 5, 1, 20);
	
	grayscaleImage = Mat(camera.getHeight(), camera.getWidth(), 1);
	
	gui.init();
	auto cameraPanel = gui.add(camera, "Camera");
	auto widgets = gui.addScroll();
	
	widgets->add(Widgets::Title::make("Chessboard sketch", Widgets::Title::Level::H1));
	widgets->add(Widgets::makeFps());
	
	auto widthSlider = Widgets::Slider::make(this->sizeWidth);
	auto heightSlider = Widgets::Slider::make(this->sizeHeight);
	widgets->add(widthSlider);
	widgets->add(heightSlider);
	widthSlider->addIntValidator();
	widthSlider->addIntValidator();
	
	widgets->add(Widgets::LiveValue<int>::make("Found boards", [this] () {
		return (int) this->corners.size();
	}));
	widgets->add(Widgets::Button::make("Add board [ ]", [this] () {
		this->keyPressed(' ');
	}));
	widgets->add(Widgets::Button::make("Clear boards [c]", [this] () {
		this->keyPressed('c');
	}));
	widgets->add(Widgets::Button::make("Calibrate [RETURN]", [this] () {
		this->keyPressed(OF_KEY_RETURN);
	}));
	cameraPanel->onDrawCropped += [this] (ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
		ofxCv::drawCorners(this->currentCorners);
	};
}

//--------------------------------------------------------------
void ofApp::update(){
	camera.update();
	
	try{
		auto incomingImage = toCv(this->camera);
		if (incomingImage.empty()){
			throw(std::exception());
		}
		cv::cvtColor(incomingImage, this->grayscaleImage, CV_RGB2GRAY);
		ofxCv::copy(camera.getPixelsRef(), grayscaleImage);
		

		findChessboardCornersPreTest(this->grayscaleImage, cv::Size(this->sizeWidth, this->sizeHeight), toCv(this->currentCorners));
	} catch (std::exception e) {
		cout << e.what();
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
		case ' ':
			this->corners.push_back(this->currentCorners);
			break;
		case 'c':
			this->currentCorners.clear();
			break;
		case OF_KEY_RETURN:
		{
			Mat camera, distortion;
			vector<Point3f> rotations, translations;
			const auto boardPoints = vector<vector<Point3f>>(this->corners.size(), ofxCv::makeBoardPoints(cv::Size(9, 5), 1.0f));
			cv::calibrateCamera(boardPoints, toCv(this->corners), cv::Size(this->camera.getWidth(), this->camera.getHeight()), camera, distortion, rotations, translations);
			
			FileStorage fs(ofToDataPath("calibration.yml"), FileStorage::WRITE);
			fs << "camera" << camera;
			fs << "distortion" << distortion;
			fs << "rotations" << rotations;
			fs << "translations" << translations;
			
			fs.release();
			
			break;
		}
		default:
			break;
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

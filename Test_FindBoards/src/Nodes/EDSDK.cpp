#include "EDSDK.h"

#include "ofxCvGui.h"

using namespace ofxDigitalEmulsion;
using namespace ofxCvGui;

//--------
EDSDK::EDSDK() {

}

//--------
void EDSDK::init() {
	auto group = MAKE(Panels::Groups::Grid);
	auto liveViewView = MAKE(Panels::Draws, this->camera.getLiveTexture());
	auto photoView = MAKE(Panels::Image, this->image);
	liveViewView->setCaption("Live View");
	photoView->setCaption("Photo");
	group->add(liveViewView);
	group->add(photoView);
	this->view = group;

	this->camera.setup();
}

//--------
string EDSDK::getTypeName() const {
	return "EDSDK";
}

//--------
Graph::PinSet EDSDK::getInputPins() const {
	return this->inputPins;
}

//--------
PanelPtr EDSDK::getView() {
	return this->view;
}

//--------
void EDSDK::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
	inspector->add(MAKE(ofxCvGui::Widgets::Button, "Take photo", [this]() {
		this->camera.takePhoto();
	}));
}

//--------
void EDSDK::update() {
	this->camera.update();
	if (this->camera.isPhotoNew()) {
		auto & pixels = this->camera.getPhotoPixels();
		this->image.setFromPixels(pixels);
	}
}
#pragma once

#include "ofxDigitalEmulsion/Graph/Node.h"

#include "ofxEdsdk.h"

class EDSDK : public ofxDigitalEmulsion::Graph::Node {
public:
	EDSDK();
	void init() override;
	string getTypeName() const override;
	ofxDigitalEmulsion::Graph::PinSet getInputPins() const override;
	void serialize(Json::Value &) override { };
	void deserialize(const Json::Value &) override { };

	ofxCvGui::PanelPtr getView() override;
	void populateInspector2(ofxCvGui::ElementGroupPtr) override;
	void update();
protected:
	ofxDigitalEmulsion::Graph::PinSet inputPins;
	ofxCvGui::PanelPtr view;

	ofxEdsdk::Camera camera;
	ofImage image;
};
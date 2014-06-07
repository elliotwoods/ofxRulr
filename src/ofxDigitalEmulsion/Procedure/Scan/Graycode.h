#pragma once

#include "../Base.h"

#include "../../../addons/ofxGraycode/src/ofxGraycode.h"
#include "ofxCvGui/Panels/Image.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Scan {
			class Graycode : public Procedure::Base {
			public:
				Graycode();
				string getTypeName() const override;
				Graph::PinSet getInputPins() override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &) override;
				void deserialize(Json::Value &) override;

				bool isReady();
				void runScan();
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr);
				void switchIfLookingAtDirtyView();

				Graph::PinSet inputPins;
				shared_ptr<ofxCvGui::Panels::Image> view;

				ofxGraycode::PayloadGraycode payload;
				ofxGraycode::Encoder encoder;
				ofxGraycode::Decoder decoder;
				ofImage message;

				ofImage preview;
				ofParameter<float> threshold;
				ofParameter<float> delay;

				bool previewIsOfNonLivePixels;
			};
		}
	}
}
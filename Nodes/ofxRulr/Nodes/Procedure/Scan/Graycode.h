#pragma once

#include "../Base.h"

#include "../../../addons/ofxGraycode/src/ofxGraycode.h"
#include "ofxCvGui/Panels/Image.h"

namespace ofxRulr {
	namespace Procedure {
		namespace Scan {
			class Graycode : public Procedure::Base {
			public:
				Graycode();
				void init();
				string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
 
				bool isReady();
				void runScan();

				ofxGraycode::Decoder & getDecoder();
				const ofxGraycode::DataSet & getDataSet() const;

			protected:
				void drawPreviewOnVideoOutput(const ofRectangle &);
				void populateInspector(ofxCvGui::ElementGroupPtr);
				void switchIfLookingAtDirtyView();

				shared_ptr<ofxCvGui::Panels::Image> view;

				ofxGraycode::PayloadGraycode payload;
				ofxGraycode::Encoder encoder;
				ofxGraycode::Decoder decoder;
				ofImage message;

				ofImage preview;
				ofParameter<float> threshold;
				ofParameter<float> delay;
				ofParameter<float> brightness;
				ofParameter<bool> enablePreviewOnVideoOutput;

				bool previewIsOfNonLivePixels;
			};
		}
	}
}
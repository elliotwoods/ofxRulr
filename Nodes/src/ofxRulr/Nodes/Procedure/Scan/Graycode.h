#pragma once

#include "../Base.h"

#include "ofxGraycode.h"
#include "ofxCvGui/Panels/Image.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Scan {
				class Graycode : public Procedure::Base {
				public:
					enum VideoOutputMode {
						None = 0,
						TestPattern = 1,
						Data = 2
					};
					
					enum PreviewMode {
						CameraInProjector = 0,
						ProjectorInCamera = 1,
						Median = 2,
						MedianInverse = 3,
						Active = 4
					};
					
					Graycode();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					bool isReady();
					void runScan();
					void clear();

					ofxGraycode::Decoder & getDecoder();
					const ofxGraycode::DataSet & getDataSet() const;

				protected:
					void drawPreviewOnVideoOutput(const ofRectangle &);
					void populateInspector(ofxCvGui::InspectArguments &);
					void updatePreview();
					void updateTestPattern();
					string getPreviewModeString() const;
					
					ofxCvGui::PanelPtr view;

					ofxGraycode::PayloadGraycode payload;
					ofxGraycode::Encoder encoder;
					ofxGraycode::Decoder decoder;
					ofImage message;
					
					ofParameter<float> threshold;
					ofParameter<float> delay;
					ofParameter<float> brightness;
					
					// check enum PreviewMode for reference
					ofParameter<int> videoOutputMode;
					ofParameter<int> previewMode;
					
					ofTexture testPattern;
					ofTexture preview;
					uint8_t testPatternBrightness = 0;
					bool previewDirty = true;
				};
			}
		}
	}
}
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
					MAKE_ENUM(VideoOutputMode
						, (None, TestPattern, Data)
						, ("None", "TestPattern", "Data"));
					
					MAKE_ENUM(PreviewMode
						, (CameraInProjector, ProjectorInCamera, Median, MedianInverse, Active)
						, ("CinP", "PinC", "Med", "MedIn", "Active"));

					Graycode();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void throwIfNotReadyForScan() const;
					void runScan();
					void clear();
					bool hasData() const;

					bool hasDecoder() const;
					ofxGraycode::Decoder & getDecoder() const;
					const ofxGraycode::DataSet & getDataSet() const;
					void setDataSet(const ofxGraycode::DataSet &);
				protected:
					void rebuild();
					void drawPreviewOnVideoOutput(const ofRectangle &);
					void populateInspector(ofxCvGui::InspectArguments &);
					void updatePreview();
					void updateTestPattern();
					string getPreviewModeString() const;
					
					ofxCvGui::PanelPtr view;

					unique_ptr<ofxGraycode::PayloadGraycode> payload;
					unique_ptr<ofxGraycode::Encoder> encoder;
					unique_ptr<ofxGraycode::Decoder> decoder;
					ofImage message;
					
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> captureDelay{ "Capture delay [ms]", 200 };
							ofParameter<float> flushFrames{ "Flush frames", 1 };
							ofParameter<float> brightness{ "Brightness [/255]", 255, 0, 255 };
							PARAM_DECLARE("Scan", captureDelay, flushFrames, brightness);
						} scan;

						struct : ofParameterGroup {
							ofParameter<float> threshold{ "Threshold", 10, 0, 255 };
							PARAM_DECLARE("Processing", threshold)
						} processing;

						struct : ofParameterGroup {
							ofParameter<VideoOutputMode> videoOutputMode{ "Video output mode", VideoOutputMode::Options::TestPattern };
							ofParameter<PreviewMode> previewMode{ "Preview mode", PreviewMode::Options::CameraInProjector };

							PARAM_DECLARE("Preview", videoOutputMode, previewMode);
						} preview;

						PARAM_DECLARE("Graycode", scan, processing, preview);
					} parameters;
					
					ofTexture testPattern;
					ofTexture preview;
					uint8_t testPatternBrightness = 0;
					bool previewDirty = true;
					bool shouldLoadWhenReady = false;

					void callbackChangeThreshold(float &);
					void callbackChangePreviewMode(PreviewMode &);
				};
			}
		}
	}
}
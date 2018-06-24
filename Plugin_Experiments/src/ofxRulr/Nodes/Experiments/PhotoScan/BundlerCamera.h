#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace PhotoScan {
				class BundlerCamera : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						void drawWorld();
						string getDisplayString() const override;

						ofParameter<float> f{ "f", 0.0f };
						ofParameter<float> k1{ "k1", 0.0f };
						ofParameter<float> k2{ "k2", 0.0f };

						vector<cv::Point3f> worldPoints;
						vector<cv::Point2f> imagePoints;
						vector<ofFloatColor> tiePointColors;

						ofMesh preview;
					};

					BundlerCamera();
					string getTypeName() const override;
					void init();
					void addCapture();
					void calibrate();
					ofxCvGui::PanelPtr getPanel() override;
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();

					void importBundler(const string & filePath);
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> dotSize{ "Dot Size", 5, 1, 256 };
							PARAM_DECLARE("Preview", dotSize);
						} preview;

						struct : ofParameterGroup {
							ofParameter<int> decimator{ "Decimator", 1, 1, 256 };
							PARAM_DECLARE("Calibrate", decimator);
						} calibrate;

						PARAM_DECLARE("BundlerCamera", preview, calibrate);
					} parameters;

					ofxCvGui::PanelPtr panel;

					Utils::CaptureSet<Capture> captures;

					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0, 0, 100.0 };

					ofFbo previewTilePoints;
				};
			}
		}
	}
}
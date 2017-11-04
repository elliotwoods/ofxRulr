#pragma once

#include "ofxCvGui/Panels/Draws.h"
#include "ofxRulr/Nodes/Procedure/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectorFromKinectV2 : public ofxRulr::Nodes::Procedure::Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
						string getDisplayString() const override;

						vector<ofVec2f> cameraImagePoints;
						vector<ofVec3f> worldSpace;
						vector<ofVec2f> projectorImageSpace;
					};

					ProjectorFromKinectV2();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void addCapture();
					void calibrate();
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();
					shared_ptr<ofxCvGui::Panels::Draws> view;

					struct : ofParameterGroup {
						ofParameter<float> checkerboardScale{ "Checkerboard Scale", 0.2f, 0.01f, 1.0f };
						ofParameter<float> checkerboardCornersX{ "Checkerboard Corners X", 5, 1, 10 };
						ofParameter<float> checkerboardCornersY{ "Checkerboard Corners Y", 4, 1, 10 };
						ofParameter<float> checkerboardPositionX{ "Checkerboard Position X", 0, -1, 1 };
						ofParameter<float> checkerboardPositionY{ "Checkerboard Position Y", 0, -1, 1 };
						ofParameter<float> checkerboardBrightness{ "Checkerboard Brightness", 0.5, 0, 1 };

						ofParameter<float> initialLensOffset{ "Initial Lens Offset", 0.5f, -1.0f, 1.0f };
						ofParameter<bool> trimOutliers{ "Trim Outliers", false };
					} parameters;
					

					Utils::CaptureSet<Capture> captures;
					float error;
				};
			}
		}
	}
}
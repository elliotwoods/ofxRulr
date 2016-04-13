#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace Orbbec {
					class ToProjector : public Nodes::Base {
					public:
						struct Correspondence {
							ofVec3f world;
							ofVec2f projector;
						};

						ToProjector();
						string getTypeName() const override;

						void init();
						void populateInspector(ofxCvGui::InspectArguments &);
						void drawWorld();
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);

						void addCapture();
						void calibrate();
					protected:
						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<float> scale{ "Scale", 0.25f, 0.01f, 1.0f };
								ofParameter<int> cornersX{ "Corners X", 5 };
								ofParameter<int> cornersY{ "Corners Y", 4 };
								ofParameter<float> positionX{ "Position X", 0.0f, -1.0f, 1.0f };
								ofParameter<float> positionY{ "Position Y", 0.0f, -1.0f, 1.0f };
								ofParameter<float> brightness{ "Brightness", 0.5f, 0.0f, 1.0f };
								PARAM_DECLARE("Checkerboard", scale, cornersX, cornersY, positionX, positionY, brightness);
							} checkerboard;

							struct : ofParameterGroup {
								ofParameter<float> initialLensOffset{ "Initial lens offset", 0.5f, -2.0f, 2.0f };
								ofParameter<float> initialThrowRatio{ "Initial throw ratio", 1.4f, 0.1f, 10.0f };
								PARAM_DECLARE("Calibration", initialLensOffset, initialThrowRatio);
							} calibration;

							PARAM_DECLARE("ToProjector", checkerboard, calibration);
						} parameters;

						ofParameter<float> error{ "Error", 0.0f };
						void drawVideoOutput();

						vector<Correspondence> correspondences;
					};
				}
			}
		}
		
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/PolyFit.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ProjectCircle : public Nodes::Base {
				public:
					struct Calibration {
						glm::vec2 center;
						ofxRulr::Utils::PolyFit::Model polyFit;

						double angleCenter;
						double tangentAngle; // angle away from the center

						pair<glm::vec2, glm::vec2> previewImageExtremities[3];
						pair<glm::vec2, glm::vec2> previewImageCircleLines[3];
					};

					ProjectCircle();
					string getTypeName() const override;

					void init();
					void update();

					ofxCvGui::PanelPtr getPanel() override;

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					void populateInspector(ofxCvGui::InspectArguments &);

					void calibrateAndDrawCircle();
				protected:
					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						ofParameter<float> laserY{ "Laser Y", 0, -1, +1 };

						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<float> x{ "X", 1000, 0, 10000 };
								ofParameter<float> y{ "Y", 1000, 0, 10000 };
								PARAM_DECLARE("Position", x, y);
							} position;
							ofParameter<float> radius{ "Radius", 500, 0, 2000 };
							ofParameter<bool> enableDragging{ "Enable dragging", false };
							PARAM_DECLARE("Circle", position, radius, enableDragging);
						} circle;

						struct : ofParameterGroup {
							ofParameter<float> range{ "Range [%]", 0.7f, 0.0f, 1.0f };
							ofParameter<bool> keepRound1{ "Keep round 1", false }; // use the 'round 1' captures for final fit 
							PARAM_DECLARE("Fitting", range, keepRound1);
						} fitting;

						PARAM_DECLARE("ProjectCircle", laserY, circle, fitting);
					} parameters;

					ofImage preview;
					unique_ptr<Calibration> calibration;
				};
			}
		}
	}
}
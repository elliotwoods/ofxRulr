#pragma once
#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class SunCalibrator : public Nodes::Base {
				public:
					struct DrawOptions {
						bool cameraRay;
						bool cameraView;
						bool solarVectorObjectSpace;
						glm::mat4 sunTrackerTransform;
					};

					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						string getDisplayString() const override;
						void serialize(nlohmann::json &) const;
						void deserialize(const nlohmann::json &);
						void drawWorld(const DrawOptions&);

						glm::vec2 centroidUndistorted;
						glm::vec2 centroidDistorted;
						ofxRay::Camera cameraView;
						ofxRay::Ray cameraRay;
						vector<glm::vec2> contour;
						ofRectangle bounds;
					protected:
						ofxCvGui::ElementPtr getDataDisplay() override;
					};

					SunCalibrator();
					string getTypeName() const override;

					void init();
					void drawWorldStage();
					ofxCvGui::PanelPtr getPanel() override;

					void populateInspector(ofxCvGui::InspectArguments&);
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);

					void addFromFiles();
					void calibrate();
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> navigateCamera{ "Navigate camera", true };
							ofParameter<int> threshold{"Threshold", 200, 0, 255};
							PARAM_DECLARE("Capture", navigateCamera, threshold);
						} capture;

						struct : ofParameterGroup {
							ofParameter<int> maxIterations {"Max iterations", 5000 };
							ofParameter<float> functionTolerance {"Function tolerance", 1e-8 };
							ofParameter<bool> printOutput{"Print output", true};
							PARAM_DECLARE("Solver", maxIterations, functionTolerance, printOutput);
						} solver;

						struct : ofParameterGroup {
							ofParameter<bool> cameraRay{"Camera ray", true};
							ofParameter<bool> cameraView{"Camera view", true};
							ofParameter<bool> solarVector{"Solar vector", true};
							PARAM_DECLARE("Draw", cameraRay, cameraView, solarVector);
						} draw;
						PARAM_DECLARE("SunCalibrator", capture, solver, draw);
					} parameters;

					Utils::CaptureSet<Capture> captures;
					
					shared_ptr<ofxCvGui::Panels::Image> panel;
					ofImage preview;
				};
			}
		}
	}
}
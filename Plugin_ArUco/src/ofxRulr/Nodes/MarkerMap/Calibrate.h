#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include <aruco/aruco.h>
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Solvers/MarkerProjections.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			class PLUGIN_ARUCO_EXPORTS Calibrate : public Nodes::Base {
			public:
				class Capture : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Capture();
					string getDisplayString() const override;

					void drawWorld();

					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					vector<int> IDs;
					vector<vector<glm::vec2>> imagePoints;
					
					ofxRay::Camera cameraView;
					vector<ofxRay::Ray> cameraRays;
				};

				Calibrate();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;

				void capture();
				void calibrate();
			protected:
				void add(const cv::Mat& image);
				void addFolderOfImages();

				Utils::CaptureSet<Capture> captures;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> maxIterations{ "Max iterations", 1000 };
							ofParameter<float> functionTolerance{ "Function tolerance", 1e-8 };
							ofParameter<bool> useIncompleteSolution{ "Use imcomplete solution",false };
							PARAM_DECLARE("Bundle Adjustment", enabled, maxIterations, functionTolerance, useIncompleteSolution);
						} bundleAdjustment;
						PARAM_DECLARE("Calibration", bundleAdjustment);
					} calibration;

					struct : ofParameterGroup {
						ofParameter<float> cameraFarPlane{ "Camera far plane", 0.2f, 0.01f, 10.0f };
						ofParameter<float> cameraRayLength{ "Camera ray length", 10.0f, 0.2f, 50.0f};
						ofParameter<bool> unpackInitial{ "Unpack initial", false };
						PARAM_DECLARE("Debug", cameraFarPlane, cameraRayLength, unpackInitial);
					} debug;

					struct : ofParameterGroup {
						ofParameter<bool> cameraRays{ "Camera rays", true };
						ofParameter<bool> cameraViews{ "Camera views", true };
						PARAM_DECLARE("Draw", cameraRays, cameraViews);
					} draw;

					PARAM_DECLARE("Calibrate", calibration, debug, draw);
				} parameters;
			};
		}
	}
}
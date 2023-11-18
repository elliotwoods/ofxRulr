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
					ofxCvGui::ElementPtr getDataDisplay() override;

					void drawWorld();

					virtual string getName() const override;
					void serialize(nlohmann::json&) const;
					void deserialize(const nlohmann::json&);

					ofParameter<string> name{ "Name", "" };
					vector<int> IDs;
					vector<vector<glm::vec2>> imagePoints;
					vector<vector<glm::vec2>> imagePointsUndistorted;
					vector<float> residuals;
					bool initialised = false;

					ofxRay::Camera cameraView;
					vector<ofxRay::Ray> cameraRays;

					Calibrate* parent;
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
				void calibrateSelected();
				void calibrateProgressiveMarkers();
				void calibrateProgressiveMarkersContinuously();

			protected:
				void add(const cv::Mat& image, const string& name);
				void addFolderOfImages();

				void unpackSolution(vector<shared_ptr<Capture>> captures
					, Solvers::MarkerProjections::Solution& solution
					, vector<shared_ptr<Markers::Marker>>& markers
					, const vector<Solvers::MarkerProjections::Image>& images);

				void initialiseCaptureViewWithSeenMarkers(shared_ptr<Capture>);
				void initialiseUnseenMarkersInView(shared_ptr<Capture>);

				Utils::CaptureSet<Capture> captures;
				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> maxIterations{ "Max iterations", 10000 };
							ofParameter<float> functionTolerance{ "Function tolerance", 1e-9 };
							ofParameter<bool> useIncompleteSolution{ "Use imcomplete solution",true };
							ofParameter<int> numThreads{ "Number of threads",4 };
							PARAM_DECLARE("Bundle Adjustment", enabled, maxIterations, functionTolerance, useIncompleteSolution, numThreads);
						} bundleAdjustment;
						PARAM_DECLARE("Calibration", bundleAdjustment);
					} calibration;

					struct : ofParameterGroup {
						ofParameter<float> maximumResidual{ "Maximum residual", 1e3 };
						struct : ofParameterGroup {
							ofParameter<int> minSeenMarkers{ "Minimum seen markers", 3 };
							ofParameter<int> maxCapturesToAdd{ "Maximum captures to add", 2 };
							ofParameter<int> maxTriesContinuous{ "Max tries continuous", 500 };
							PARAM_DECLARE("Progressive Markers", minSeenMarkers, maxCapturesToAdd, maxTriesContinuous);
						} progressiveMarkers;
						PARAM_DECLARE("Progressive calibration", maximumResidual, progressiveMarkers);
					} progressiveCalibration;

					struct : ofParameterGroup {
						ofParameter<float> cameraFarPlane{ "Camera far plane", 0.2f, 0.01f, 10.0f };
						ofParameter<float> cameraRayLength{ "Camera ray length", 10.0f, 0.2f, 50.0f};
						ofParameter<bool> unpackInitial{ "Unpack initial", false };
						PARAM_DECLARE("Debug", cameraFarPlane, cameraRayLength, unpackInitial);
					} debug;

					struct : ofParameterGroup {
						ofParameter<bool> cameraRays{ "Camera rays", true };
						ofParameter<bool> cameraViews{ "Camera views", true };
						ofParameter<bool> labels{ "Labels", true };
						PARAM_DECLARE("Draw", cameraRays, cameraViews, labels);
					} draw;

					PARAM_DECLARE("Calibrate", calibration, progressiveCalibration, debug, draw);
				} parameters;
			};
		}
	}
}
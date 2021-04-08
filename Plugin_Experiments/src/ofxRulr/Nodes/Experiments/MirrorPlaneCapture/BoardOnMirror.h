#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class BoardOnMirror : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						void drawWorld();
						string getDisplayString() const override;

						struct {
							bool enabled;
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> worldPoints;
							glm::vec3 cameraPosition;
							ofParameter<float> reprojectionError{ "Reprojection error", 0 };
						} cameraNavigation;

						struct Plane {
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> objectPoints;

							ofxRay::Plane plane;
							float reprojectionError;
							float planeFitResidual;
							glm::mat4 boardTransform;
							vector<glm::vec3> worldPoints;

							glm::vec3 meanObjectPoint;
							glm::vec3 meanWorldPoint;
						};

						Plane mirrorPlane;

						string name;
						string heliostatName;

						void serialize(nlohmann::json &);
						void deserialize(const nlohmann::json &);
					protected:
						void drawPlane(const Plane &) const;
					};

					BoardOnMirror();
					string getTypeName() const override;
					void init();
					void update();
					void addCapture();
					void calibrate();
					ofxCvGui::PanelPtr getPanel() override;
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();

					void addImage(const cv::Mat &, const string & name = "");
					void addFolderOfImages(const std::filesystem::path & path);

					void copyMeanPlaneToClipboard() const;

					void projectToHeliostats();

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> minimumMarkers{ "Minimum markers", 3 };
							ofParameter<float> maxMarkerError{ "Max marker error", 0.5f };
							PARAM_DECLARE("Camera navigation", enabled, minimumMarkers, maxMarkerError);
						} cameraNavigation;

						struct : ofParameterGroup {
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Raw };
							ofParameter<int> minimumCorners{ "Minimum corners", 8 };
							PARAM_DECLARE("Capture", findBoardMode, minimumCorners);
						} capture;

						struct : ofParameterGroup {
							ofParameter<float> planeToA2{ "Plane to A2  [m]", 0.15f };
							ofParameter<float> distanceThreshold{ "Distance threshold [m]", 0.2f };
							ofParameter<int> minimumCaptures{ "Minimum captures", 2 };
							PARAM_DECLARE("Heliostat projection", distanceThreshold, minimumCaptures);
						} heliostatProjection;

// 						struct : ofParameterGroup {
// 							ofParameter<WhenDrawOnWorldStage> navigation{ "Navigation", WhenDrawOnWorldStage::Selected };
// 							PARAM_DECLARE("Draw", navigation);
// 						} draw;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<string> address{ "Address", "http://localhost:8000/Calibrate/LogServoValues" };
							PARAM_DECLARE("Log to server", enabled, address);
						} logToServer;

						PARAM_DECLARE("SolveMirror", cameraNavigation, capture, heliostatProjection, logToServer);
					} parameters;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					Utils::CaptureSet<Capture> captures;
				};
			}
		}
	}
}
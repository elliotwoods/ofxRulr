#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class BoardInMirror : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						void drawWorld();
						string getDisplayString() const override;

						struct {
							bool enabled;
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> worldPoints;
							ofVec3f cameraPosition;
							ofParameter<float> reprojectionError{ "Reprojection error", 0 };
						} cameraNavigation;

						struct Plane {
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> objectPoints;

							ofxRay::Plane plane;
							float reprojectionError;
							float planeFitResidual;
							ofMatrix4x4 boardTransform;
							vector<ofVec3f> worldPoints;
							ofVec3f worldPointCenter;

							ofVec3f meanObjectPoint;
							ofVec3f testPointWorld;
						};

						Plane realPlane;
						Plane virtualPlane;

						ofVec3f testObjectPoint;
						ofVec3f estimatedPointOnMirrorPlane;
						ofxRay::Plane mirrorPlane;

						string name;
					protected:
						void drawPlane(const Plane &) const;
					};

					BoardInMirror();
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
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> minimumMarkers{ "Minimum markers", 3 };
							PARAM_DECLARE("Camera navigation", enabled, minimumMarkers);
						} cameraNavigation;

						struct : ofParameterGroup {
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Raw };
							ofParameter<int> minimumCorners{ "Minimum corners", 8 };
							PARAM_DECLARE("Capture", findBoardMode, minimumCorners);
						} capture;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<string> address{ "Address", "http://localhost:8000/Calibrate/LogServoValues"};
							PARAM_DECLARE("Log to server", enabled, address);
						} logToServer;

						PARAM_DECLARE("SolveMirror", cameraNavigation, capture, logToServer);
					} parameters;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					Utils::CaptureSet<Capture> captures;
				};
			}
		}
	}
}
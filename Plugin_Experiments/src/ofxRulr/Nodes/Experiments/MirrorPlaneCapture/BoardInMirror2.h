#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Solvers/MirrorPlaneFromRays.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				MAKE_ENUM(FlipImage
					, (None, X, Y)
					, ("None", "X", "Y"));

				/*
				Navigate the camera using the MarkerMap
				*/
				class BoardInMirror2 : public Base {
				public:
					/* 
					* Each capture represents a photo capture, and may include data for more than one heliostat.
					* For now we only support one heliostat
					* cameraRays see reflections of worldPoints (taken from teh board) in the mirror plane
					*/
					class PhotoCapture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						PhotoCapture();

						void drawWorld();
						string getDisplayString() const override;
						void serialize(nlohmann::json&);
						void deserialize(const nlohmann::json&);

						struct {
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> worldPoints;
							float reprojectionError = 0.0f;
							glm::vec3 cameraPosition;
						} cameraNavigation;

						vector<cv::Point2f> imagePoints;
						vector<cv::Point3f> objectPoints;
						vector<cv::Point3f> worldPoints;
						vector<ofxRay::Ray> cameraRays;
					};

					BoardInMirror2();
					string getTypeName() const override;
					void init();
					void update();
					void capture();
					void calibrate();
					ofxCvGui::PanelPtr getPanel() override;
					void populateInspector(ofxCvGui::InspectArguments&);
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					void drawWorldStage();

				protected:
					void addCapture(shared_ptr<ofxMachineVision::Frame>);

					struct : ofParameterGroup {
						ofParameter<WhenDrawOnWorldStage> tetheredShootEnabled{ "Tethered shoot enabled", WhenDrawOnWorldStage::Selected };

						struct : ofParameterGroup {
							ofParameter<int> minimumMarkers{ "Minimum markers", 1 };
							PARAM_DECLARE("Tracking", minimumMarkers);
						} cameraNavigation;

						struct : ofParameterGroup {
							ofParameter<FindBoardMode> mode{ "Mode", FindBoardMode::Assistant };
							ofParameter<FlipImage> flipImage{ "Flip image", FlipImage::X };
							PARAM_DECLARE("Find board", mode, flipImage);
						} findBoard;

						struct : ofParameterGroup {
							ofParameter<int> maxIterations{ "Max iterations", 1000 };
							ofParameter<bool> printReport{ "Print report", true };
							PARAM_DECLARE("Solve", maxIterations, printReport);
						} solve;

						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<bool> reflectedRays{ "Reflected rays", true};
								PARAM_DECLARE("Draw", reflectedRays);
							} draw;
							PARAM_DECLARE("Debug", draw);
						} debug;

						PARAM_DECLARE("BoardInMirror2"
							, tetheredShootEnabled
							, cameraNavigation
							, findBoard
							, solve
							, debug);
					} parameters;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					Utils::CaptureSet<PhotoCapture> captures;

					shared_ptr<ofxRulr::Solvers::MirrorPlaneFromRays::Result> result;
					
					struct {
						vector<ofxRay::Ray> reflectedRays;
					} debug;
				};
			}
		}
	}
}
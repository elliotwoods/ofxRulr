#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Solvers/MirrorPlaneFromRays.h"
#include "Heliostats2.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ExtrinsicsFromBoardInWorld.h"

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
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						struct DrawOptions {
							bool camera;
							bool cameraRays;
							bool worldPoints;
							bool reflectedRays;
							bool mirrorFace;
						};

						Capture();

						void drawWorld(const DrawOptions&);
						string getDisplayString() const override;
						void serialize(nlohmann::json&);
						void deserialize(const nlohmann::json&);
						
						string heliostatName;
						string comments;
						glm::vec3 cameraPosition;

						vector<cv::Point2f> imagePoints;
						vector<cv::Point3f> worldPoints;
						ofxRay::Camera camera;
						vector<ofxRay::Ray> cameraRays;
						int axis1ServoPosition;
						int axis2ServoPosition;

						vector<float> residuals;
						float meanResidual = 0.0f;

						vector<ofxRay::Ray> reflectedRays;
						glm::vec3 mirrorCenter;
						glm::vec3 mirrorNormal;

						const float mirrorDiameter = 0.35f;
					};

					BoardInMirror2();
					string getTypeName() const override;
					void init();
					void update();

					void capture();
					void navigateToSeeBoardAndCapture(vector<shared_ptr<Heliostats2::Heliostat>> activeHeliostats
						, shared_ptr<Heliostats2> heliostats
						, shared_ptr<Procedure::Calibrate::ExtrinsicsFromBoardInWorld>
						, const ofxCeres::SolverSettings& navigateSolverSettings
						, shared_ptr<Item::BoardInWorld> boardInWorld
						, shared_ptr<Item::Camera> camera
						, const ofxRay::Camera& cameraView
						, shared_ptr<Dispatcher> dispatcher
						, const string& comments);

					void processReflectionInHeliostat(shared_ptr<Heliostats2>
						, shared_ptr<Heliostats2::Heliostat>
						, const ofxRay::Camera& cameraView
						, const cv::Mat& imageWithReflectionsWithoutRealBoard
						, shared_ptr<Item::BoardInWorld>
						, const map<Dispatcher::ServoID, Dispatcher::RegisterValue>&
						, const string & comments
						, float meanBrightness);


					void addCapture(shared_ptr<ofxMachineVision::Frame>);
					void calibrate();

					ofxCvGui::PanelPtr getPanel() override;
					void populateInspector(ofxCvGui::InspectArguments&);
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					void drawWorldStage();

					void filterCapturesToHeliostatSelection();
					void calculateResiduals();
				protected:
					ofxCeres::SolverSettings getNavigateSolverSettings() const;
					ofxCeres::SolverSettings getCalibrateSolverSettings() const;

					struct : ofParameterGroup {
						ofParameter<WhenDrawOnWorldStage> tetheredShootEnabled{ "Tethered shoot enabled", WhenDrawOnWorldStage::Selected };
						
						struct : ofParameterGroup {
							ofParameter<bool> beforeFindBoardDirect{ "Before find board direct", true };
							ofParameter<bool> waitAtStart{ "Wait at start", false };
							ofParameter<bool> atEnd{ "At end", true };
							PARAM_DECLARE("Face away", beforeFindBoardDirect, waitAtStart, atEnd);
						} faceAway;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<bool> trustPriorPose{ "Trust prior pose", true };
							PARAM_DECLARE("Camera Navigation", enabled, trustPriorPose);
						} cameraNavigation;

						struct : ofParameterGroup {
							ofParameter<bool> atStart{ "At start", false };
							ofParameter<FindBoardMode> modeForReflections{ "Mode for reflections", FindBoardMode::Optimized };
							ofParameter<bool> useAssistantIfFail{ "Use assistant if fail", true };
							ofParameter<bool> updateBoardInFinalImage{ "Update board in final image", false };
							PARAM_DECLARE("Find board", atStart, modeForReflections, useAssistantIfFail, updateBoardInFinalImage);
						} findBoard;

						struct : ofParameterGroup {
							ofParameter<float> waitTime{ "Wait time", 10.0f, 0.0f, 30.0f };
							PARAM_DECLARE("Servo control", waitTime);
						} servoControl;

						struct : ofParameterGroup {
							ofParameter<float> mirrorScale{ "Mirror scale (mask)", 1.0f, 0.8f, 2.0f };
							ofParameter<bool> flip{ "Flip", true };

							// Alternatively aim to board center
							ofParameter<bool> aimToSeenBoardPoints{ "Aim to seen board points", true };
							PARAM_DECLARE("Capture", mirrorScale, flip, aimToSeenBoardPoints);
						} capture;

						struct : ofParameterGroup {
							ofParameter<int> maxIterations{ "Max iterations", 5000 };
							ofParameter<bool> printReport{ "Print report", true };
							PARAM_DECLARE("Navigate", maxIterations, printReport);
						} navigate;

						struct : ofParameterGroup {
							ofParameter<int> minimumDataPoints{ "Minimum data points", 100 };
							ofParameter<bool> fixPosition{ "Fix position", false };
							ofParameter<bool> fixRotationY{ "Fix rotation Y", true};
							ofParameter<bool> fixPolynomial{ "Fix polynomial", true };
							ofParameter<bool> fixRotationAxis{ "Fix rotation axis", true };
							ofParameter<bool> fixMirrorOffset{ "Fix mirror offset", true };
							ofParameter<int> maxIterations{ "Max iterations", 1000 };
							ofParameter<float> functionTolerance{ "Function tolerance", 1e-8 };
							ofParameter<float> parameterTolerance{ "Parameter tolerance", 1e-7 };
							ofParameter<bool> printReport{ "Print report", true };
							PARAM_DECLARE("Calibrate"
								, minimumDataPoints
								, fixPosition
								, fixRotationY
								, fixPolynomial
								, fixRotationAxis
								, fixMirrorOffset
								, maxIterations
								, functionTolerance
								, parameterTolerance
								, printReport);
						} calibrate;


						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<bool> cameras{ "Cameras", true };
								ofParameter<bool> cameraRays{ "Camera rays", true };
								ofParameter<bool> worldPoints{ "World points", true };
								ofParameter<bool> reflectedRays{ "Reflected rays", true };
								ofParameter<bool> mirrorFace{ "Mirror face", true };
								ofParameter<bool> report{ "Report", true };
								PARAM_DECLARE("Draw", cameraRays, worldPoints, reflectedRays, mirrorFace, report);
							} draw;
							PARAM_DECLARE("Debug", draw);
						} debug;

						PARAM_DECLARE("BoardInMirror2"
							, tetheredShootEnabled
							, faceAway
							, cameraNavigation
							, findBoard
							, servoControl
							, capture
							, navigate
							, calibrate
							, debug);
					} parameters;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					Utils::CaptureSet<Capture> captures;
				};
			}
		}
	}
}
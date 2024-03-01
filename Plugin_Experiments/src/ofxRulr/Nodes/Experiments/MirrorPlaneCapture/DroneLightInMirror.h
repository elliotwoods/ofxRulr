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
				/*
				Navigate the camera using the MarkerMap
				*/
				class DroneLightInMirror : public Base {
				public:
					/*
					* Each capture represents a photo capture, and may include data for more than one heliostat.
					* For now we only support one heliostat
					* cameraRays see reflections of worldPoints (taken from teh board) in the mirror plane
					*/
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						struct DrawOptions {
							set<string> selectedHelisotats;

							bool selectedHeliostatsOnly;
							bool camera;
							bool cameraRays;
							bool cameraRaysShort;
							bool lightPositions;
							float worldPointsSize;
							bool labelWorldPoints;
							bool reflectedRays;
							bool mirrorFace;
						};

						struct HeliostatCapture {
							string heliostatName;
							glm::vec2 imagePoint; // distorted
							ofxRay::Ray cameraRay;

							glm::vec3 mirrorCenterEstimated;
							glm::vec3 mirrorNormalEstimated;
							ofxRay::Ray reflectedRayEstimated;

							int axis1ServoPosition;
							int axis2ServoPosition;

							float axis1AngleOffest = 0.0f;
							float axis2AngleOffest = 0.0f;

							float residual = 0.0f;
						};

						Capture();

						void drawWorld(const DrawOptions&);
						string getDisplayString() const override;
						void serialize(nlohmann::json&);
						void deserialize(const nlohmann::json&);

						string comments;
						glm::vec3 cameraPosition;
						glm::vec3 lightPosition;
						ofxRay::Camera camera;
						
						vector<HeliostatCapture> heliostatCaptures;
						float meanResidual = 0.0f;
					};

					DroneLightInMirror();
					string getTypeName() const override;
					void init();
					void update();
					void populateInspector(ofxCvGui::InspectArguments&);
					ofxCvGui::PanelPtr getPanel() override;
					void drawWorldStage();

					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					void navigateToSeeLight();
					void navigateToSeeLight(vector<shared_ptr<Heliostats2::Heliostat>> heliostats
						, shared_ptr<Heliostats2> heliostatsNode
						, const ofxCeres::SolverSettings& navigateSolverSettings);
					
					void capture();
					void calibrate();

					void filterCapturesToHeliostatSelection();
					void calculateResiduals();

					void filterHeliostatsToInsideCamera();
				protected:
					static vector<shared_ptr<Heliostats2::Heliostat>> getHeliostatsInView(vector<shared_ptr<Heliostats2::Heliostat>> heliostats
						, const ofxRay::Camera& cameraView);

					glm::vec3 getLightPosition(const glm::mat4& cameraTransform);

					ofxCeres::SolverSettings getNavigateSolverSettings() const;
					ofxCeres::SolverSettings getCalibrateSolverSettings() const;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> x{ "X", 0, -1, 1 };
							ofParameter<float> y{ "Y", 0.02, -1, 1 };
							ofParameter<float> z{ "Z", -0.125, -1, 1 };
							PARAM_DECLARE("Light position", x, y, z);
						} lightPosition;

						struct : ofParameterGroup {
							ofParameter<bool> whenSeeLight{ "When see light", true };
							ofParameter<bool> whenCapture{ "When capture", true };
							ofParameter<bool> trustPriorPose{ "Trust prior pose", true };
							PARAM_DECLARE("Camera Navigation", whenSeeLight, whenCapture, trustPriorPose);
						} cameraNavigation;

						struct : ofParameterGroup {
							ofParameter<bool> onFreshFrame{ "On fresh frame", true };
							ofParameter<int> maxIterations{ "Max iterations", 5000 };
							ofParameter<bool> printReport{ "Print report", true };
							PARAM_DECLARE("Navigate", onFreshFrame, maxIterations, printReport);
						} navigate;

						struct : ofParameterGroup {
							ofParameter<bool> waitForNewFrame{ "Wait for new frame", true };
							ofParameter<bool> debugFindLight{ "Debug find light", true };
							PARAM_DECLARE("Capture", waitForNewFrame, debugFindLight);
						} capture;

						struct : ofParameterGroup {
							ofParameter<int> minimumDataPoints{ "Minimum data points", 8 };
							ofParameter<bool> fixPosition{ "Fix position", false };
							ofParameter<bool> fixRotationY{ "Fix rotation Y", true };
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
								ofParameter<bool> selectedHeliostatsOnly { "Selected heliostats only", true };
								ofParameter<bool> light{ "Light", true };
								ofParameter<bool> cameras{ "Cameras", true };
								ofParameter<bool> cameraRays{ "Camera rays", false };
								ofParameter<bool> cameraRaysShort{ "Camera rays short", true };
								ofParameter<bool> lightPositions{ "Light positions", true };
								ofParameter<float> worldPointsSize{ "World points size", 0.01, 1e-3, 1e3 };
								ofParameter<bool> labelWorldPoints{ "Label world points", false };
								ofParameter<bool> reflectedRays{ "Reflected rays", true };
								ofParameter<bool> mirrorFace{ "Mirror face", true };
								ofParameter<bool> report{ "Report", true };
								PARAM_DECLARE("Draw", selectedHeliostatsOnly, light, cameras, cameraRays, cameraRaysShort, lightPositions, labelWorldPoints, worldPointsSize, reflectedRays, mirrorFace, report);
							} draw;
							PARAM_DECLARE("Debug", draw);
						} debug;

						PARAM_DECLARE("DroneLightInMirror"
							, lightPosition
							, cameraNavigation
							, capture
							, navigate
							, calibrate
							, debug);
					} parameters;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					Utils::CaptureSet<Capture> captures;

					// This is used for tethered capture on navigate to see light (but ignore if previous frame was capture)
					bool lastFrameWasCapture = false;
				};
			}
		}
	}
}
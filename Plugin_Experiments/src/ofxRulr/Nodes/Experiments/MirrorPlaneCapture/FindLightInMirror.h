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

				class FindLightInMirror : public Base
				{
				public:
					MAKE_ENUM(Mode
						, (Interactive, NonInteractive)
						, ("Interactive", "Non interactive"));

					struct FindSettings {
						bool buildPreview = false;
					};

					struct Result {
						bool success = false;
						glm::vec2 imagePoint;
						string message;
					};

					FindLightInMirror();
					string getTypeName() const override;

					void init();
					void update();
					void populateInspector(ofxCvGui::InspectArguments& args);
					ofxCvGui::PanelPtr getPanel();

					Result findLight(const cv::Mat& image
						, const ofxRay::Camera& cameraView
						, shared_ptr<Heliostats2> heliostatsNode
						, shared_ptr<Heliostats2::Heliostat>
						, const FindSettings& = FindSettings());

					vector<Result> findLights(const cv::Mat& image
						, vector<shared_ptr<Heliostats2::Heliostat>>
						, shared_ptr<Heliostats2> heliostatsNode
						, const FindSettings & = FindSettings());

				protected:
					Result findLightInteractive(const cv::Mat& image
						, const ofxRay::Camera& cameraView
						, shared_ptr<Heliostats2> heliostatsNode
						, shared_ptr<Heliostats2::Heliostat>
						, const FindSettings & = FindSettings());

					Result findLightRoutine(const cv::Mat& image
						, const ofxRay::Camera& cameraView
						, shared_ptr<Heliostats2> heliostatsNode
						, shared_ptr<Heliostats2::Heliostat>
						, const FindSettings & = FindSettings()
						, const glm::vec2& closestToInMirror = glm::vec2(0, 0));

					void testFindLights();
					void refreshPreview();

					struct : ofParameterGroup {
						ofParameter<Mode> mode {"Mode", Mode::Interactive};

						struct : ofParameterGroup {
							ofParameter<int> scaleUpWindow{ "Scale up window [^2]", 0, 0, 3 };
							PARAM_DECLARE("Interactive window", scaleUpWindow);
						} interactiveMode;

						struct : ofParameterGroup {
							ofParameter<bool> waitForNewFrame{ "Wait for new frame", false };
							PARAM_DECLARE("Camera", waitForNewFrame);
						} camera;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<float> scale{ "Scale (mirror)", 1.0f, 0.8f, 6.0f };
							ofParameter<int> minDimension{ "Min dimension", 128 };
							PARAM_DECLARE("Mask", enabled, scale, minDimension);
						} mask;

						struct : ofParameterGroup {
							ofParameter<float> blur{ "Blur [mirror]", 0.2f, 0.01f, 1.0f };
							ofParameter<bool> blurIterative{ "Blur iterative", false};
							ofParameter<float> threshold{ "Threshold", 127, 0, 255 };
							ofParameter<int> removeNoise{ "Remove noice [steps]", 3 };
							ofParameter<float> maxArea{ "Max area [mirror]", 1, 0, 1 }; // 1 = ignore
							ofParameter<int> dilateForMoments{ "Dilate for moments", 3 };
							PARAM_DECLARE("Detection", blur, blurIterative, threshold, removeNoise, maxArea, dilateForMoments)
						} detection;
						
						PARAM_DECLARE("FindLightInMirror"
							, mode
							, interactiveMode
							, camera
							, mask
							, detection);
					} parameters;

					struct {
						bool dirty = true;

						cv::Mat croppedImage;
						cv::Mat croppedMask;
						cv::Mat maskedImage;
						cv::Mat background;
						cv::Mat difference;
						cv::Mat binary;

						ofImage croppedImagePreview;
						ofImage croppedMaskPreview;
						ofImage maskedImagePreview;
						ofImage backgroundPreview;
						ofImage differencePreview;
						ofImage binaryPreview;


						ofPolyline blobOutline;
						ofRectangle momentsBounds;
						glm::vec2 pointInCropped;
					} preview;

					shared_ptr<ofxCvGui::Panels::Groups::Grid> panel;
				};
			}
		}
	}
}
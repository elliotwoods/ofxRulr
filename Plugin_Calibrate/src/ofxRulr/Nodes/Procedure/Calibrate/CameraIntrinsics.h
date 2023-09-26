#pragma once

#include "ofxRulr.h"
#include "ofxCvMin.h"
#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class CameraIntrinsics : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();

						string getDisplayString() const override;
						void drawWorldStage();
						void drawOnImage() const;

						vector<glm::vec2> pointsImageSpace;
						vector<glm::vec3> pointsObjectSpace;

						ofParameter<float> imageWidth{ "Image width", 0.0f };
						ofParameter<float> imageHeight{ "Image height", 0.0f };

						ofParameter<glm::mat4> extrsinsics{ "Extrinsics", glm::mat4(1.0f) };
						ofParameter<float> reprojectionError{ "Reprojection error", 0.0f };
					protected:
						void serialize(nlohmann::json &);
						void deserialize(const nlohmann::json &);
					};

					CameraIntrinsics();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;

					void update();
					void drawWorldStage();

					void remoteControl(RemoteControllerArgs&);

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);

					bool getRunFinderEnabled() const;

					// Manually add an image to the capture set
					void addImage(cv::Mat);

					// Add a folder of images to the capture set
					void addFolder(const std::filesystem::path & path);
				protected:
					void populateInspector(ofxCvGui::InspectArguments &);
					void addCapture(bool triggeredFromTetheredCapture);
					void findBoard();
					void calibrate();

					shared_ptr<ofxCvGui::Panels::BaseImage> view;
					ofTexture preview;

					Utils::CaptureSet<Capture> captures;

					vector<glm::vec2> currentImagePoints;
					vector<glm::vec3> currentObjectPoints;

					ofParameter<float> error{ "Reprojection error [px]", 0.0f };

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> checkAllIncomingFrames{ "Check all incoming frames", true };
							ofParameter<WhenActive> tetheredShootEnabled{ "Tethered shoot enabled", WhenActive::Selected };
							ofParameter<FindBoardMode> findBoardMode{ "Mode", FindBoardMode::Optimized };

							PARAM_DECLARE("Capture", checkAllIncomingFrames, tetheredShootEnabled, findBoardMode);
						} capture;
						PARAM_DECLARE("CameraIntrinsics", capture);
					} parameters;
					
					bool isFrameNew = false;
				};
			}
		}
	}
}
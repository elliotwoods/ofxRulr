#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Utils/VideoOutputListener.h"

#include "Constants_Plugin_Calibration.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class PLUGIN_CALIBRATE_EXPORTS ProjectorFromGraycode : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						string getDisplayString() const override;
						vector<glm::vec3> worldPoints;
						vector<glm::vec2> cameraImagePoints;
						vector<glm::vec2> projectorImagePoints;
						vector<glm::vec2> reprojectedProjectorImagePoints;
						glm::mat4 transform;

						void serialize(nlohmann::json &);
						void deserialize(const nlohmann::json &);
						void drawWorld(shared_ptr<Item::Projector>);

						void drawOnCameraImage();
						void drawOnProjectorImage();
					};

					ProjectorFromGraycode();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;

					void init();
					void update();
					void drawWorldStage();
					
					void addCapture();
					void calibrate();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
				protected:
					void drawOnVideoOutput(const ofRectangle & bounds);

					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> useExistingGraycodeScan{ "Use existing graycode scan", false };
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
							ofParameter<float> pixelSearchDistance { "Pixel search distance", 3, 0, 10 };

							PARAM_DECLARE("Capture"
								, useExistingGraycodeScan
								, findBoardMode
								, pixelSearchDistance);
						} capture;

						struct : ofParameterGroup {
							ofParameter<float> initialLensOffset{ "Initial lens offset", 0, -1.0, 1.0 };
							ofParameter<float> initialThrowRatio{ "Initial throw ratio", 1, 0, 5 };
							ofParameter<bool> useDecimation{ "Use decimation", false };
							struct : ofParameterGroup {
								ofParameter<bool> enabled{ "Enabled", false };
								ofParameter<float> maxReprojectionError{ "Max reprojection error [px]", 10.0f };
								PARAM_DECLARE("Remove outliers", enabled, maxReprojectionError);
							} removeOutliers;

							PARAM_DECLARE("Calibrate", initialLensOffset, initialThrowRatio, useDecimation, removeOutliers);
						} calibrate;

						struct : ofParameterGroup {
							ofParameter<bool> dataOnVideoOutput{ "Data on VideoOutput", true };
							ofParameter<bool> reprojectedOnVideoOutput{ "Reprojected points on VideoOutput", true };

							PARAM_DECLARE("Draw", dataOnVideoOutput, reprojectedOnVideoOutput)
						} draw;

						PARAM_DECLARE("ProjectorFromGraycode", capture, calibrate, draw);
					} parameters;

					Utils::CaptureSet<Capture> captures;
					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0 };
					unique_ptr<Utils::VideoOutputListener> videoOutputListener;

					struct {
						ofTexture projectorInCamera;
						ofTexture cameraInProjector;
					} preview;
				};
			}
		}
	}
}
#pragma once

#include "ofxRulr.h"
#include "ofxCvMin.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class PLUGIN_CALIBRATE_EXPORTS ExtrinsicsFromBoardInWorld : public Base {
				public:
					MAKE_ENUM(UpdateTarget
						, (Camera, Board)
						, ("Camera", "Board"));

					ExtrinsicsFromBoardInWorld();
					void init();
					string getTypeName() const override;
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					void drawWorldStage();

					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);
					void populateInspector(ofxCvGui::InspectArguments&);

					void track(const UpdateTarget &, bool getFreshFrame);
				protected:
					void receiveFrame(shared_ptr<ofxMachineVision::Frame>);
					ofxCvGui::PanelPtr view;

					ofImage image;
					vector<cv::Point2f> imagePoints;
					vector<cv::Point3f> objectPoints;
					vector<cv::Point3f> worldPoints;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
							ofParameter<WhenDrawOnWorldStage> tetheredShootEnabled{ "Tethered shoot enabled", WhenDrawOnWorldStage::Selected };
							PARAM_DECLARE("Capture", findBoardMode, tetheredShootEnabled);
						} capture;

						struct : ofParameterGroup {
							ofParameter<bool> useExtrinsicGuess{ "Use extrinsic guess", false };
							ofParameter<bool> trackOnFreshFrame{ "Track on fresh frame", true };
							ofParameter<UpdateTarget> updateTarget{ "Update target", UpdateTarget::Camera };
							PARAM_DECLARE("Track", useExtrinsicGuess, trackOnFreshFrame, updateTarget);
						} track;

						struct : ofParameterGroup {
							ofParameter<bool> rays{ "Rays", true };
							PARAM_DECLARE("Draw", rays);
						} draw;

						PARAM_DECLARE("ExtrinsicsFromBoardInWorld", capture, track, draw);
					} parameters;
				};
			}
		}
	}
}
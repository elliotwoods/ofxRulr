#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

//temporarilly here whilst we have the function as member
#include "ofxRulr/Nodes/Procedure/Calibrate/StereoCalibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class StereoSolvePnP : public Nodes::Base {
			public:
				StereoSolvePnP();
				string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
				void drawWorld();
				
				bool solvePnPStereo(shared_ptr<Procedure::Calibrate::StereoCalibrate> stereoCalibrateNode
					, const vector<cv::Point2f> & imagePointsA
					, const vector<cv::Point2f> & imagePointsB
					, const vector<cv::Point3f> & objectPointsA
					, const vector<cv::Point3f> & objectPointsB
					, cv::Mat & rotationVector
					, cv::Mat & translation
					, bool useExtrinsicGuess);

			protected:
				struct : ofParameterGroup {
					ofParameter<FindBoardMode> findBoardMode{ "Mode", FindBoardMode::Optimized };

					struct : ofParameterGroup {
						ofParameter<bool> a{ "A", true };
						ofParameter<bool> b{ "B", true };
						ofParameter<bool> stereo{ "Stereo", true };
						PARAM_DECLARE("Draw", a, b, stereo);
					} draw;

					PARAM_DECLARE("StereoSolvePnP", findBoardMode, draw);
				} parameters;

				struct {
					ofTexture previewB;
					ofTexture previewA;
					vector<cv::Point2f> imagePointsA;
					vector<cv::Point2f> imagePointsB;
					ofMatrix4x4 transformA;
					ofMatrix4x4 transformB;
					ofMatrix4x4 transformStereoResult;
				} dataPreview;

				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
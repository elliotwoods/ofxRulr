#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class FindLine : public Nodes::Base {
				public:
					struct OutputFrame {
						ofPixels foregroundPixels;
						ofPixels backgroundPixels;

						cv::Mat foreground;
						cv::Mat background;

						cv::Mat difference;
						cv::Mat binary;
						cv::Mat maskedDifference;

						vector<cv::Point2f> points;
						cv::Vec4f lineFirstTry;

						vector<cv::Point2f> filteredPoints;
						cv::Vec4f line;
					};

					FindLine();
					string getTypeName() const override;

					void init();
					void update();

					ofxCvGui::PanelPtr getPanel() override;

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::InspectArguments &);

					shared_ptr<OutputFrame> findLine(ofPixels foreground, ofPixels background);
				protected:
					void capture();
					void recalculate();

					ofxCvGui::PanelPtr panel;

					struct : ofParameterGroup {
						ofParameter<bool> useLastPhoto{ "Use last photo", false };

						struct : ofParameterGroup {
							ofParameter<float> differenceAmplify{ "Difference amplify", 4, 1, 64 };
							ofParameter<float> threshold{ "Threshold", 30, 0, 255 };
							ofParameter<float> erosionSize{ "Erosion size", 1, 0, 10 };
							PARAM_DECLARE("Local Difference", differenceAmplify, threshold, erosionSize);
						} localDifference;

						struct : ofParameterGroup {
							ofParameter<bool> enableVerticalBoxes{ "Enabled vertical boxes", false };
							ofParameter<int> boxSize{ "Box size", 16, 2, 512 };
							ofParameter<int> minimumCountInBox{ "Minimum count in box", 2 };
							ofParameter<float> distanceThreshold{ "Distance threshold", 8, 0, 32 };
							PARAM_DECLARE("Line Fit", boxSize, minimumCountInBox, distanceThreshold);
						} lineFit;

						PARAM_DECLARE("FindLines", useLastPhoto, localDifference, lineFit);
					} parameters;

					shared_ptr<OutputFrame> previewFrame;
					weak_ptr<OutputFrame> cachedPreviewFrame;

					ofImage previewDifference;
					ofImage previewBinary;
					ofImage previewMaskedBinary;

					ofVboMesh previewPoints;
					ofVboMesh previewFilteredPoints;
				};
			}
		}
	}
}
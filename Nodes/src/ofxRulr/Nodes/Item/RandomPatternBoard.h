#pragma once

#include "AbstractBoard.h"
#include "ofxCvMin.h"
#include <opencv2/ccalib/randpattern.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class RandomPatternBoard : public AbstractBoard {
			public:
				RandomPatternBoard();
				string getTypeName() const override;

				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
				void populateInspector(ofxCvGui::InspectArguments&);

				void drawObject() const override;
				bool findBoard(cv::Mat, vector<cv::Point2f>& result, vector<cv::Point3f>& objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const override;
			protected:
				void rebuild();
				shared_ptr<ofxCvGui::Panels::Image> panel;

				struct Parameters : ofParameterGroup {
					ofParameter<float> width{ "Width [px]", 2048, 256, 1024 * 16 };
					ofParameter<float> height{ "Height [px]", 2048, 256, 1024 * 16 };

					struct : ofParameterGroup {
						ofParameter<int> nminiMatch{ "nminiMatch", 20 };
						ofParameter<int> depth{ "depth", CV_32F };
						ofParameter<int> verbose{ "verbose", 0 };
						ofParameter<int> showExtraction{ "showExtraction", 0 };
						PARAM_DECLARE("Corner finder", nminiMatch, depth, verbose, showExtraction);
					} cornerFinder;

					PARAM_DECLARE("Board", width, height, cornerFinder);
				} parameters;

				shared_ptr<cv::randpattern::RandomPatternGenerator> patternGenerator;
				shared_ptr<cv::randpattern::RandomPatternCornerFinder> cornerFinder;

				Parameters cachedBoardParameters;
				ofImage image;
			};
		}
	}
}
#pragma once

#include "pch_Plugin_SLS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace SLS {
			class Scan : public ofxRulr::Nodes::Base {
			public:
				struct Suite {
					cv::structured_light::GrayCodePattern::Params params;
					cv::Ptr<cv::structured_light::GrayCodePattern> pattern;
					vector<cv::Mat> patternImages;
					cv::Mat whiteProjector;
					cv::Mat blackProjector;
				};

				struct DataSet {
					shared_ptr<Suite> suite;

					int cameraWidth;
					int cameraHeight;
					vector<cv::Mat> captures;
					cv::Mat whiteCamera;
					cv::Mat blackCamera;

					ofShortPixels projectorXYInCamera;
					ofPixels mask;
				};

				Scan();
				string getTypeName() const override;
				void init();

				shared_ptr<DataSet> capture();
				void decode(shared_ptr<DataSet>);

				void scan();
				shared_ptr<Suite> getSuite(); // throws ofxRulr::Exception
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> brightness{ "Brightness", 1, 0, 1 };
						ofParameter<int> flushOutputFrames{ "Flush output frames", 2 };
						ofParameter<int> flushInputFrames{ "Flush input frames", 0 };
						ofParameter<int> captureDelay{ "Capture delay [ms]", 100 };
						PARAM_DECLARE("Scan", flushOutputFrames, flushInputFrames, captureDelay);
					} scan;

					struct : ofParameterGroup {
						ofParameter<float> white{ "White", 20 };
						ofParameter<float> black{ "Black", 5 };
						PARAM_DECLARE("Threshold", white, black);
					} threshold;
					
					PARAM_DECLARE("Scan", scan, threshold);
				} parameters;

				shared_ptr<DataSet> dataSet;
				ofImage message;
			};
		}
	}
}
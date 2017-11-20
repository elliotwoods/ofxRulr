#pragma once

#include "ofxCvGui.h"
#include "ofxCvMin.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			class HistogramWidget : public ofxCvGui::Element {
			public:
				HistogramWidget();
				void draw(ofxCvGui::DrawArguments & args);
				void setData(const cv::Mat &);
			protected:
				ofPolyline graph;
			};
		}
	}
}
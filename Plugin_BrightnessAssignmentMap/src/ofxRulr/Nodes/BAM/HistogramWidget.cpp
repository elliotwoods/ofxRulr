#include "pch_Plugin_BrightnessAssignmentMap.h"
#include "HistogramWidget.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			using namespace ofxCvGui;

			//----------
			HistogramWidget::HistogramWidget() {
				this->onDraw += [this](DrawArguments & args) {
					this->draw(args);
				};
				this->setHeight(80.0f);
			}

			//----------
			void HistogramWidget::draw(ofxCvGui::DrawArguments & args) {
				ofPushMatrix();
				{
					ofTranslate(args.localBounds.getBottomLeft());
					ofScale(args.localBounds.width / 32.0f, - args.localBounds.height);
					ofDrawLine(ofVec2f(16, 0), ofVec2f(16, 1));
					this->graph.draw();
				}
				ofPopMatrix();
			}

			//----------
			void HistogramWidget::setData(const cv::Mat & histogram) {
				this->graph.clear();
				
				const auto size = 32; // using this as constant
				double maximumValue = 0;
				cv::minMaxLoc(histogram, 0, &maximumValue, 0, 0);

				this->graph.addVertex(ofVec2f(0, 0));

				for (int i = 0; i < size; i++) {
					auto value = histogram.at<float>(i) / maximumValue;
					this->graph.addVertex(ofVec2f((float) i / (float) 32.0f, value));
				}
			}
		}
	}
}
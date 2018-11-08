#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolvePnP {
				class SolvePnP : public Base {
				public:
					SolvePnP();
					string getTypeName() const override;
					void init();
					void update();
					void drawWorldStage();
					void populateInspector(ofxCvGui::InspectArguments &);
				protected:
					cv::Mat rotationVector, translation;

					struct {
						cv::Mat rotationVector, translation;
					} error;

					struct {
						cv::Mat rotationVector, translation;
					} movingAverageError;
				};
			}
		}
	}
}
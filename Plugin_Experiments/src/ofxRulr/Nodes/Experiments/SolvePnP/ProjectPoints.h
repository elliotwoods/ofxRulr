#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolvePnP {
				class ProjectPoints : public Base {
				public:
					ProjectPoints();
					string getTypeName() const override;
					void init();
					void update();
					ofxCvGui::PanelPtr getPanel() override;

					const vector<ofVec2f> & getProjectedPoints();
				protected:
					struct : ofParameterGroup {
						ofParameter<float> noise{ "Noise [px]", 0, 0, 100.0 };
						PARAM_DECLARE("ProjectPoints", noise);
					} parameters;
					
					shared_ptr<ofxCvGui::Panels::Draws> panel;
					ofFbo fbo;

					vector<ofVec2f> projectedPoints;
				};
			}
		}
	}
}
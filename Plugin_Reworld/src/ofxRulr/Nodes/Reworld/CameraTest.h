#pragma once

#include "ofxRulr.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class CameraTest: public Nodes::Base {
			public:
				CameraTest();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);
				ofxCvGui::PanelPtr getPanel() override;
			protected:
				shared_ptr<ofxCvGui::Panels::Image > panel;
				ofImage preview;
				vector<glm::vec2> sourceCorners;
				vector<glm::vec2> targetCorners;
				int selectedCornerIndex = 0;

				glm::mat4 homography;
				bool transformDirty = true;
			};
		}
	}
}
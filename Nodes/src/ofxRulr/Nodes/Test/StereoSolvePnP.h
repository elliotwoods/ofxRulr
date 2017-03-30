#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			class StereoSolvePnP : public Nodes::Base {
			public:
				StereoSolvePnP();
				string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
				void drawWorld();
				
			protected:
				struct : ofParameterGroup {
					ofParameter<Item::AbstractBoard::FindBoardMode> findBoardMode{ "Mode", Item::AbstractBoard::FindBoardMode::Optimized };
					PARAM_DECLARE("StereoSolvePnP", findBoardMode);
				} parameters;

				ofxCvGui::PanelPtr panel;
				ofMatrix4x4 transform;
			};
		}
	}
}
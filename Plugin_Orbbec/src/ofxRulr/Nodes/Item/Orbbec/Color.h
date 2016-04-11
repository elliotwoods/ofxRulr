#pragma once

#include "ofxRulr/Nodes/Item/View.h"
#include "Device.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				class Color : public Item::View {
				public:
					Color();
					string getTypeName() const override;

					void init();
					void update();

					ofxCvGui::PanelPtr getPanel() override;
				protected:
					ofxCvGui::PanelPtr panel;
					ofTexture texture;
				};
			}
		}
	}
}
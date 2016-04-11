#pragma once

#include "ofxRulr/Nodes/Item/View.h"
#include "Device.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				class Infrared : public Item::View {
				public:
					Infrared();
					string getTypeName() const override;

					void init();
					void update();
					ofxCvGui::PanelPtr getPanel() override;

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void calcIntrinsicsUsingDepthToWorld();
				protected:
					struct {
						float reprojectionError = 0.0f;
					} results;
					ofxCvGui::PanelPtr panel;
					ofTexture texture;
				};
			}
		}
	}
}
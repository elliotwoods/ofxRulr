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

					ofVec3f cameraToDepth(const ofVec2f & camera);
					ofVec3f cameraToWorld(const ofVec2f & camera);
				protected:
					struct : ofParameterGroup {
						ofParameter<bool> renderDepthMap{ "Render depth map", false };
						PARAM_DECLARE("Color", renderDepthMap);
					} parameters;

					void renderDepthMap();

					shared_ptr<ofxCvGui::Panels::Groups::Strip> panelStrip;
					ofTexture texture;

					ofFbo drawPointCloud;
					ofFbo extractDepthBuffer;
					ofFloatPixels colorDepthBufferPixels;
				};
			}
		}
	}
}
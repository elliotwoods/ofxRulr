#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class ImageBox : public Nodes::Item::RigidBody {
			public:
				ImageBox();
				string getTypeName() const override;

				void init();
				void update();

				void drawObject();

				virtual ofxCvGui::PanelPtr getPanel() override;

			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> width{ "Width [m]", 4.0, 0.0, 20.0 };
						ofParameter<float> height{ "Height [m]", 4.0, 0.0, 20.0 };
						ofParameter<float> depth{ "Depth [m]", 4.0, 0.0, 20.0 };
						PARAM_DECLARE("Box", width, height, depth);
					} box;

					struct : ofParameterGroup {
						ofParameter<int> width{ "Width [px]", 1024 };
						ofParameter<int> height{ "Height [px]", 512 };
						PARAM_DECLARE("Resolution", width, height);
					} resolution;

					struct : ofParameterGroup {
						ofParameter<bool> subscriberIndex{ "Subscriber index", false };
						ofParameter<bool> IR{ "IR", false };
						PARAM_DECLARE("Draw style", subscriberIndex, IR);
					} drawStyle;

					PARAM_DECLARE("ImageBox", box, resolution, drawStyle)
				} parameters;

				ofFbo fbo;
				shared_ptr<ofxCvGui::Panels::Groups::Strip> panel;
			};
		}
	}
}
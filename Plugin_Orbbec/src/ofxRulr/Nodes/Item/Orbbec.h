#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxOrbbec.h"

#include "ofxCvGui/Panels/Groups/Strip.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Orbbec : public Nodes::Item::RigidBody {
			public:
				Orbbec();
				string getTypeName() const override;

				void init();
				void update();

				void drawObject();

				ofxCvGui::PanelPtr getPanel() override;
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> color {"Color", true};
						ofParameter<bool> depth{ "Depth", true };
						ofParameter<bool> points{ "Points", true };
						ofParameter<bool> skeleton{ "Skeleton", true };
						PARAM_DECLARE("Enabled streams", color, depth);
					} enabledStreams;
					PARAM_DECLARE("Orbbec", enabledStreams);
				} parameters;

				shared_ptr<ofxCvGui::Panels::Groups::Strip> panelStrip;
				shared_ptr<ofxOrbbec::Device> device;
			};
		}
	}
}
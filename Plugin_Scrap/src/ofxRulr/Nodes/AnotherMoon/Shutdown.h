#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Shutdown : public Nodes::Item::RigidBody {
			public:
				Shutdown();
				string getTypeName() const override;
				
				void init();
				ofxCvGui::PanelPtr getPanel() override;

				void shutdown();
			protected:
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Target : public Nodes::Base {
			public:
				Target();
				string getTypeName() const override;
				void init();
				void drawWorldStage();
				void populateInspector(ofxCvGui::InspectArguments&);

				void positionTargetRelativeToLasers();
				void pointLasersTowardsTarget();
			};
		}
	}
}
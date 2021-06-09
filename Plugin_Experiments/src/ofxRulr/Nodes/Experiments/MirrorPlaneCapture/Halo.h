#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class Halo : public Item::RigidBody {
				public:
					Halo();
					string getTypeName() const override;

					void init();
					void update();

					void drawObject();
				protected:
					struct : ofParameterGroup {
						ofParameter<float> diameter{"Diameter"}
						PARAM_DECLARE("Halo", diameter);
					};
				};
			}
		}
	}
}
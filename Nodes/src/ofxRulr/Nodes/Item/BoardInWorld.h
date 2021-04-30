#pragma once

#include "RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class BoardInWorld : public RigidBody {
			public:
				BoardInWorld();
				string getTypeName() const override;
			protected:
				void init();
				void update();
			};
		}
	}
}
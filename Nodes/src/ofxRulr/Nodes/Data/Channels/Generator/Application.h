#pragma once

#include "Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			namespace Channels {
				namespace Generator{
					class Application : public Base {
					public:
						Application();
						string getTypeName() const override;
						void populateData(Channel &) override;
					};
				}
			}
		}
	}
}
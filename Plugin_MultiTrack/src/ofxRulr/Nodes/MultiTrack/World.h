#pragma once

#include "Receiver.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class World : public Nodes::Base {
			public:
				enum Constants : size_t {
					NumReceivers = 6
				};

				World();
				string getTypeName() const override;
				void init();
				void update();

			protected:
				map<size_t, weak_ptr<Receiver>> receivers;
			};
		}
	}
}
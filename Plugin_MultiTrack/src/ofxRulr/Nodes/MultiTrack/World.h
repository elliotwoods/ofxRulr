#pragma once

#include "Subscriber.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class World : public Nodes::Base {
			public:
				enum Constants : size_t {
					NumSubscribers = 6
				};

				World();
				string getTypeName() const override;
				void init();
				void update();

				map<size_t, weak_ptr<Subscriber>> & getSubscribers();

			protected:
				map<size_t, weak_ptr<Subscriber>> subscribers;
			};
		}
	}
}
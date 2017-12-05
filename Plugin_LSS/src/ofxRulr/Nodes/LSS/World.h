#pragma once

#include "Projector.h"
#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class World : public Nodes::Base {
			public:
				World();
				string getTypeName() const override;
				void init();

				void add(shared_ptr<Projector>);
				void remove(shared_ptr<Projector>);

				vector<shared_ptr<Projector>> getProjectors() const;
			protected:
				vector<weak_ptr<Projector>> projectors;
			};
		}
	}
}
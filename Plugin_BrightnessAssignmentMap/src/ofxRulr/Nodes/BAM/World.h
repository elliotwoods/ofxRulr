#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			class Projector;
			
			class World : public Nodes::Base {
			public:
				World();
				string getTypeName() const override;
				void init();
				void registerProjector(shared_ptr<Projector>);
				void unregisterProjector(shared_ptr<Projector>);
				vector<shared_ptr<Projector>> getProjectors() const;
			protected:
				vector<weak_ptr<Projector>> projectors;
			};
		}
	}
}
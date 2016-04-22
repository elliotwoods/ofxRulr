#pragma once

#include "Subscriber.h"
#include "Utils.h"

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
				void drawWorld();

				void populateInspector(ofxCvGui::InspectArguments &);
				map<size_t, weak_ptr<Subscriber>> & getSubscribers();

			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> mergeDistanceThreshold{ "Merge distance threshold", 0.3, 0.0, 5.0 };
						PARAM_DECLARE("Fusion", enabled);
					} fusion;
					PARAM_DECLARE("World", fusion);
				} parameters;

				Subscribers subscribers;

				CombinedBodySet combinedBodies;

				typedef vector<ofxKinectForWindows2::Data::Body> Bodies;
				typedef map<SubscriberID, Bodies> WorldBodiesUnmerged;

				void performFusion();
				WorldBodiesUnmerged getWorldBodiesUnmerged() const;
				CombinedBodySet combineWorldBodies(WorldBodiesUnmerged worldBodiesUnmerged) const;
			};
		}
	}
}
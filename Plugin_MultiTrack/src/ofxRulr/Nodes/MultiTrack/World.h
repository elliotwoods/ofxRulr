#pragma once

#include "Subscriber.h"
#include "Utils.h"

#include "ofxRulr/Data/Channels/Channel.h"

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
				void drawSubscriberPointClouds() const;

				void populateInspector(ofxCvGui::InspectArguments &);
				map<size_t, weak_ptr<Subscriber>> & getSubscribers();

				MergeSettings getMergeSettings() const;
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> mergeDistanceThreshold{ "Merge distance threshold", 0.3, 0.0, 5.0 };
						ofParameter<bool> crossoverEnabled{ "Crossover enabled", true };
						ofParameter<float> crossoverMargin{ "Crossover margin [px]", 50 };
						PARAM_DECLARE("Fusion", enabled, mergeDistanceThreshold, crossoverEnabled, crossoverMargin);
					} fusion;

					struct : ofParameterGroup {
						ofParameter<bool> bodyPerimeter{ "Body perimeter", true };
						ofParameter<bool> combinedBody{ "Combined body", true };
						ofParameter<bool> sourceBodies{ "Source bodies", true };
						ofParameter<bool> bodySourceRays{ "Body source rays", true };
						PARAM_DECLARE("Draw", bodyPerimeter, combinedBody, sourceBodies, bodySourceRays);
					} draw;
					PARAM_DECLARE("World", fusion, draw);
				} parameters;

				Subscribers subscribers;

				CombinedBodySet combinedBodies;

				typedef vector<ofxKinectForWindows2::Data::Body> Bodies;
				typedef map<SubscriberID, Bodies> WorldBodiesUnmerged;

				void performFusion();
				WorldBodiesUnmerged getWorldBodiesUnmerged() const;
				CombinedBodySet combineWorldBodies(WorldBodiesUnmerged worldBodiesUnmerged) const;
				void populateDatabase(Data::Channels::Channel & rootChannel);
			};
		}
	}
}
#ifdef OFXMULTITRACK_TCP

#pragma once

#include "ofxKinectForWindows2.h"

#include "Subscriber.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			typedef size_t BodyIndex;
			typedef size_t SubscriberID;

			enum DepthMapSize {
				Width = 512,
				Height = 424
			};

			struct CombinedBody {
				map<SubscriberID, ofxKinectForWindows2::Data::Body> originalBodiesWorldSpace; // index is subscriberID
				ofxKinectForWindows2::Data::Body combinedBody;
			};
			typedef map<BodyIndex, CombinedBody> CombinedBodySet;

			typedef map<SubscriberID, weak_ptr<Subscriber>> Subscribers;

			struct MergeSettings {
				bool crossoverEnabled;
				///Size in pixels at the edge of each frame to apply crossover logic
				float crossoverMargin;
			};

			float getWeight(const ofVec2f & positionInDepthMap, const MergeSettings & mergeSettings);
			ofxKinectForWindows2::Data::Body mean(const vector<ofxKinectForWindows2::Data::Body> &, const MergeSettings &);
			ofxKinectForWindows2::Data::Body mean(const map<SubscriberID, ofxKinectForWindows2::Data::Body> &, const MergeSettings &);

			float meanDistance(ofxKinectForWindows2::Data::Body &, ofxKinectForWindows2::Data::Body &, bool xzOnly);
		}
	}
}

#endif // OFXMULTITRACK_TCP
#pragma once

#include "ofxKinectForWindows2.h"

#include "Subscriber.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			typedef size_t BodyIndex;
			typedef size_t SubscriberID;

			struct CombinedBody {
				map<SubscriberID, ofxKinectForWindows2::Data::Body> originalBodiesWorldSpace; // index is subscriberID
				ofxKinectForWindows2::Data::Body mergedBody;
			};
			typedef map<BodyIndex, CombinedBody> CombinedBodySet;

			typedef map<SubscriberID, weak_ptr<Subscriber>> Subscribers;

			ofxKinectForWindows2::Data::Body mean(const vector<ofxKinectForWindows2::Data::Body> &);
			ofxKinectForWindows2::Data::Body mean(const map<SubscriberID, ofxKinectForWindows2::Data::Body> &);

			float meanDistance(ofxKinectForWindows2::Data::Body &, ofxKinectForWindows2::Data::Body &);
		}
	}
}
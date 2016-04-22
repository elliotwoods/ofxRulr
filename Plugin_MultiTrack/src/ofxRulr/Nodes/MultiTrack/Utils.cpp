#include "pch_MultiTrack.h"
#include "Utils.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			ofxKinectForWindows2::Data::Body mean(const vector<ofxKinectForWindows2::Data::Body> & bodies) {
				if (bodies.size() == 0) {
					return ofxKinectForWindows2::Data::Body();
				}

				map<JointType, ofVec3f> positions;
				map<JointType, ofQuaternion> orientations;

				//get all tracked joints
				for (auto & body : bodies) {
					for (auto & joint : body.joints) {
						if (joint.second.getTrackingState() == TrackingState_Tracked) {
							positions[joint.first] += joint.second.getPosition();
							orientations[joint.first] = joint.second.getOrientation();
						}
					}
				}

				//also use inferred for anything with nothing fully tracked
				for (auto & body : bodies) {
					for (auto & joint : body.joints) {
						if (positions.find(joint.first) == positions.end()) {
							positions[joint.first] += joint.second.getPosition();
							orientations[joint.first] = joint.second.getOrientation();
						}
					}
				}

				//accumulate the body
				auto body = bodies.front();
				for (auto & joint : body.joints) {
					_Joint rawJoint = {
						joint.first,
						(CameraSpacePoint&)(positions[joint.first] * (1.0f / (float)bodies.size())),
						TrackingState::TrackingState_Tracked
					};

					_JointOrientation rawJointOrientation = {
						joint.first,
						(Vector4&)orientations[joint.first]
					};

					joint.second.set(rawJoint, rawJointOrientation);
				}

				return body;
			}

			//----------
			ofxKinectForWindows2::Data::Body mean(const map<SubscriberID, ofxKinectForWindows2::Data::Body> & bodiesMap) {
				vector<ofxKinectForWindows2::Data::Body> bodies;
				for (auto bodyMap : bodiesMap) {
					bodies.push_back(bodyMap.second);
				}
				return mean(bodies);
			}

			//----------
			float meanDistance(ofxKinectForWindows2::Data::Body & BodyA, ofxKinectForWindows2::Data::Body & BodyB) {
				float distance = 0.0f;
				float countFound = 0;
				for (const auto & jointIt : BodyA.joints) {
					auto findInBodyB = BodyB.joints.find(jointIt.first);
					if (findInBodyB == BodyB.joints.end()) {
						ofLogError("ofxRulr::Nodes::MultiTrack::meanDistance") << "Matching joint not found in BodyB";
					}
					else {
						if (jointIt.second.getTrackingState() == TrackingState::TrackingState_Tracked && findInBodyB->second.getTrackingState() == TrackingState::TrackingState_Tracked) {
							distance += jointIt.second.getPosition().distanceSquared(findInBodyB->second.getPosition());
							countFound++;
						}
					}
				}

				return sqrt(distance / countFound);
			}
		}
	}
}

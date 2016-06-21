#include "pch_MultiTrack.h"

#include "Utils.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {

			//----------
			float getWeight(const ofVec2f & positionInDepthMap, const MergeSettings & mergeSettings) {
				if (!mergeSettings.crossoverEnabled) {
					return 1.0f;
				}

				auto scoreX = 1.0f;
				scoreX *= ofMap(positionInDepthMap.x, 0, mergeSettings.crossoverMargin, 0.01f, 1.0f, true);
				scoreX *= 1.0f - ofMap(positionInDepthMap.x, DepthMapSize::Width - mergeSettings.crossoverMargin, DepthMapSize::Width - 1, 0.0f, 0.99f, true);

				auto scoreY = 1.0f;
				scoreY *= ofMap(positionInDepthMap.y, 0, mergeSettings.crossoverMargin, 0.01f, 1.0f, true);
				scoreY *= 1.0f - ofMap(positionInDepthMap.y, DepthMapSize::Height - mergeSettings.crossoverMargin, DepthMapSize::Height - 1, 0.0f, 0.99f, true);

				return scoreX * scoreY;
			}

			//----------
			ofxKinectForWindows2::Data::Body mean(const vector<ofxKinectForWindows2::Data::Body> & bodies, const MergeSettings & mergeSettings) {
				if (bodies.size() == 0) {
					return ofxKinectForWindows2::Data::Body();
				}

				map<JointType, ofVec3f> positions;
				map<JointType, ofQuaternion> orientations;
				map<JointType, float> accumulatedWeights;

				//get all tracked joints
				for (auto & body : bodies) {
					for (auto & joint : body.joints) {
						if (joint.second.getTrackingState() == TrackingState_Tracked) {
							auto weight = getWeight(joint.second.getPositionInDepthMap(), mergeSettings);
							positions[joint.first] += joint.second.getPosition() * weight;
							orientations[joint.first] = joint.second.getOrientation();
							
							auto findAccumulatedWeight = accumulatedWeights.find(joint.first);
							if (findAccumulatedWeight == accumulatedWeights.end()) {
								accumulatedWeights[joint.first] = weight;
							}
							else {
								accumulatedWeights[joint.first] += weight;
							}
						}
					}
				}

				//also use inferred for anything with nothing fully tracked
				for (auto & body : bodies) {
					for (auto & joint : body.joints) {
						if (positions.find(joint.first) == positions.end()) {
							auto weight = getWeight(joint.second.getPositionInDepthMap(), mergeSettings);

							positions[joint.first] += joint.second.getPosition() * weight;
							orientations[joint.first] = joint.second.getOrientation();

							auto findAccumulatedWeight = accumulatedWeights.find(joint.first);
							if (findAccumulatedWeight == accumulatedWeights.end()) {
								accumulatedWeights[joint.first] = weight;
							}
							else {
								accumulatedWeights[joint.first] += weight;
							}
						}
					}
				}

				//accumulate the body
				auto body = bodies.front();
				for (auto & joint : body.joints) {
					if (accumulatedWeights[joint.first] > 0) {
						_Joint rawJoint = {
							joint.first,
							(CameraSpacePoint&)(positions[joint.first] / (float)accumulatedWeights[joint.first]),
							TrackingState::TrackingState_Tracked
						};

						_JointOrientation rawJointOrientation = {
							joint.first,
							(Vector4&)orientations[joint.first]
						};

						joint.second.set(rawJoint, rawJointOrientation, ofVec2f());
					}
				}

				return body;
			}

			//----------
			ofxKinectForWindows2::Data::Body mean(const map<SubscriberID, ofxKinectForWindows2::Data::Body> & bodiesMap, const MergeSettings & mergeSettings) {
				vector<ofxKinectForWindows2::Data::Body> bodies;
				for (auto bodyMap : bodiesMap) {
					bodies.push_back(bodyMap.second);
				}
				return mean(bodies, mergeSettings);
			}

			//----------
			float meanDistance(ofxKinectForWindows2::Data::Body & BodyA, ofxKinectForWindows2::Data::Body & BodyB, bool xzOnly) {
				float distance = 0.0f;
				float countFound = 0;
				for (const auto & jointIt : BodyA.joints) {
					auto findInBodyB = BodyB.joints.find(jointIt.first);
					if (findInBodyB == BodyB.joints.end()) {
						ofLogError("ofxRulr::Nodes::MultiTrack::meanDistance") << "Matching joint not found in BodyB";
					}
					else {
						if (jointIt.second.getTrackingState() == TrackingState::TrackingState_Tracked && findInBodyB->second.getTrackingState() == TrackingState::TrackingState_Tracked) {
							if (xzOnly) {
								distance += (jointIt.second.getPosition() * ofVec3f(1, 0, 1)).distanceSquared(findInBodyB->second.getPosition() * ofVec3f(1, 0, 1));
							}
							else {
								distance += jointIt.second.getPosition().distanceSquared(findInBodyB->second.getPosition());
							}
							countFound++;
						}
					}
				}

				return sqrt(distance / countFound);
			}
		}
	}
}

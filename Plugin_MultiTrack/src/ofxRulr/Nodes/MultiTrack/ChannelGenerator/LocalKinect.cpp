#include "pch_MultiTrack.h"
#include "LocalKinect.h"

#include "ofxRulr/Nodes/Item/KinectV2.h"

using namespace ofxRulr::Data::Channels;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace ChannelGenerator {
				//----------
				LocalKinect::LocalKinect() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string LocalKinect::getTypeName() const {
					return "MultiTrack::ChannelGenerator::LocalKinect";
				}

				//----------
				void LocalKinect::init() {
					RULR_NODE_UPDATE_LISTENER;

					this->nodesChannel = make_shared<Channel>("nodes");

					this->addInput<Item::KinectV2>();
				}

				//----------
				void LocalKinect::update() {
					
				}

				//----------
				void LocalKinect::populateData(Channel & channel) {
					auto kinectNode = this->getInput<Item::KinectV2>();
					if (kinectNode) {
						auto & kinect = kinectNode->getDevice();
						if (kinect) {
							if (kinect->isFrameNew()) {
								channel["count"] = 1;

								auto & nodeChannel = channel["0"];
								nodeChannel["position"] = kinectNode->getPosition(); /**HACK**/
								nodeChannel["orientation"] = kinectNode->getRotationQuat().asVec4(); /**HACK**/

								auto bodySource = kinect->getBodySource();
								auto & bodies = bodySource->getBodies();

								auto & bodiesChannel = nodeChannel["bodies"];
								bodiesChannel["count"] = bodies.size();

								for (int i = 0; i < bodies.size(); i++) {
									auto & bodyChannel = bodiesChannel[ofToString(i)];
									auto & body = bodies[i];

									bodyChannel["tracked"] = body.tracked;

									if (!body.tracked) {
										bodyChannel.removeSubChannel("centroid");
										bodyChannel.removeSubChannel("joints");
									}
									else {
										/**HACK**/
										auto findSpineBase = body.joints.find(JointType::JointType_SpineBase);
										if (findSpineBase != body.joints.end()) {
											bodyChannel["centroid"] = findSpineBase->second.getPosition();
										}
										else {
											bodyChannel.removeSubChannel("centroid");
										}

										auto & jointChannel = bodyChannel["joints"];
										jointChannel["count"] = body.joints.size();

										for (auto & joint : body.joints) {
											auto jointName = ofxKinectForWindows2::toString(joint.first);
											auto & jointChannel = bodyChannel[jointName];
											jointChannel["position"] = joint.second.getPosition();
											jointChannel["orientation"] = joint.second.getOrientation().asVec4();
											jointChannel["trackingState"] = joint.second.getTrackingState();
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
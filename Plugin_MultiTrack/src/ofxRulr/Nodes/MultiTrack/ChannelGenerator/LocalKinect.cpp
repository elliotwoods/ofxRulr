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

					this->frameChannel = make_shared<Channel>("Frame");

					this->addInput<Item::KinectV2>();
				}

				//----------
				void LocalKinect::update() {
					auto kinectNode = this->getInput<Item::KinectV2>();
					if (kinectNode) {
						auto & kinect = kinectNode->getDevice();
						if (kinect) {
							if (kinect->isFrameNew()) {
								auto & frameChannel = *this->frameChannel;

								auto bodySource = kinect->getBodySource();
								auto & bodies = bodySource->getBodies();
								auto & bodiesChannel = frameChannel["bodies"];

								for (int i = 0; i < bodies.size(); i++) {
									auto & bodyChannel = bodiesChannel[ofToString(i)];
									auto & body = bodies[i];

									for (auto & joint : body.joints) {
										auto jointName = ofxKinectForWindows2::toString(joint.first);
										auto & jointChannel = bodyChannel[jointName];
										jointChannel["position"] = joint.second.getPosition();
									}
								}
							}
						}
					}
				}

				//----------
				void LocalKinect::populateData(Channel & channel) {
					channel.setSubChannel(this->frameChannel);
				}
			}
		}
	}
}
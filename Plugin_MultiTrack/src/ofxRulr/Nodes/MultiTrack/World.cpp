#include "pch_MultiTrack.h"

#include "Utils.h"

#include "World.h"
#include "ofxRulr/Nodes/Data/Channels/Database.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			World::World() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string World::getTypeName() const {
				return "MultiTrack::World";
			}

			//----------
			void World::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				//RULR_NODE_SERIALIZATION_LISTENERS;

				auto databasePin = this->addInput<Data::Channels::Database>();

				databasePin->onNewConnection += [this](shared_ptr<Data::Channels::Database> database) {
					database->onPopulateData.addListener([this](Data::Channels::Channel & rootChannel) {
						this->populateDatabase(rootChannel);
					}, this);
				};
				databasePin->onDeleteConnection += [this](shared_ptr<Data::Channels::Database> database) {
					if (database) {
						database->onPopulateData.removeListeners(this);
					}
				};

				for (size_t i = 0; i < NumSubscribers; i++) {
					auto subscriberPin = this->addInput<Subscriber>("Subscriber " + ofToString(i + 1));
					subscriberPin->onNewConnection += [this, i](shared_ptr<Subscriber> subscriber) {
						this->subscribers[i] = subscriber;
						ofxCvGui::refreshInspector(this);
					};
					subscriberPin->onDeleteConnection += [this, i](shared_ptr<Subscriber> subscriber) {
						//this is a double check. if it's empty then we might be being destroyed (which can cause subscribers to be invalid)
						if (!this->subscribers.empty()) {
							auto findSubscriber = this->subscribers.find(i);
							if (findSubscriber != this->subscribers.end()) {
								this->subscribers.erase(findSubscriber);
							}
						}

						ofxCvGui::refreshInspector(this);
					};
				}

				this->manageParameters(this->parameters);
			}

			//----------
			void World::update() {
				//perform skeleton fusion
				if(this->parameters.fusion.enabled) {
					this->performFusion();
				}
			}

			//----------
			void World::drawWorldStage() {
				ofPushStyle();
				{
					for (const auto & combinedBody : this->combinedBodies) {
						ofColor color(255, 100, 100);
						color.setHueAngle((combinedBody.first * 60) % 360);

						//draw combined body
						ofSetColor(color);
						if (this->parameters.draw.combinedBody) {
							combinedBody.second.combinedBody.drawWorld();

							//draw body label
							for (auto findReferenceJoint = combinedBody.second.combinedBody.joints.find(JointType::JointType_Head); findReferenceJoint != combinedBody.second.combinedBody.joints.end(); findReferenceJoint++) {
								//draw the first valid joint, start with the head
								ofDrawBitmapString(ofToString(combinedBody.first), findReferenceJoint->second.getPosition());
								break;
							}
						}

						//draw body perimeter
						if (this->parameters.draw.bodyPerimeter) {
							ofPushStyle();
							{
								auto dimColor = color;
								dimColor.setBrightness(50);
								ofNoFill();
								for (auto & joint : combinedBody.second.combinedBody.joints) {
									if (joint.second.getTrackingState() == TrackingState::TrackingState_Tracked) {
										ofPushMatrix();
										{
											ofTranslate(joint.second.getPosition() * ofVec3f(1, 0, 1) + ofVec3f(0, 0.005, 0));
											ofRotate(90, 1, 0, 0);
											ofDrawCircle(ofVec3f(), this->parameters.fusion.mergeDistanceThreshold / 2.0f);
										}
										ofPopMatrix();
									}
								}
							}
							ofPopStyle();
						}

						//draw original bodies and line to source kinect
						color.setBrightness(80);
						ofSetColor(color);
						for (auto originalBody : combinedBody.second.originalBodiesWorldSpace) {
							if (this->parameters.draw.sourceBodies) {
								//original body
								originalBody.second.drawWorld();
							}

							if (this->parameters.draw.bodySourceRays) {
								//line to kinect it came from
								auto findSubscriber = this->subscribers.find(originalBody.first);
								if (findSubscriber != this->subscribers.end()) {
									auto subscriber = findSubscriber->second.lock();
									if (subscriber) {
										auto findHead = originalBody.second.joints.find(JointType::JointType_Head);
										if (findHead != originalBody.second.joints.end()) {
											ofDrawLine(findHead->second.getPosition(), subscriber->getPosition());
										}
									}
								}
							}
						}
					}
				}
				ofPopStyle();
			}

			//----------
			void World::populateInspector(ofxCvGui::InspectArguments & args) {
				args.inspector->addLiveValue<size_t>("Connected subscribers", [this]() {
					return this->subscribers.size();
				});
				for (auto subscriberIt : this->subscribers) {
					auto subscriberWeak = subscriberIt.second;
					args.inspector->addLiveValueHistory("Subscriber " + ofToString(subscriberIt.first), [subscriberWeak] {
						auto subscriberNode = subscriberWeak.lock();
						if (subscriberNode) {
							auto subscriber = subscriberNode->getSubscriber();
							if (subscriber) {
								return subscriber->getSubscriber().getIncomingFramerate();
							}
						}

						return 0.0f;
					});
				}
			}


			//----------
			map<size_t, weak_ptr<Subscriber>> & World::getSubscribers() {
				return this->subscribers;
			}

			//----------
			void World::performFusion() {
				auto worldBodiesUnmerged = this->getWorldBodiesUnmerged();
				this->combinedBodies = this->combineWorldBodies(worldBodiesUnmerged);
			}

			//----------
			World::WorldBodiesUnmerged World::getWorldBodiesUnmerged() const {
				//This function accumulates all the bodies from the whole network of sensors.
				//And transforms them using the rigid body transforms from the Subscriber nodes
				WorldBodiesUnmerged worldBodiesUnmerged;

				for (auto subscriberIt : this->subscribers) {
					auto subscriberNode = subscriberIt.second.lock();
					if (subscriberNode) {
						auto subscriber = subscriberNode->getSubscriber();
						if (subscriber) {
							const auto & bodiesInCameraSpace = subscriber->getFrame().getBodies();
							auto & BodiesInWorldSpace = worldBodiesUnmerged[subscriberIt.first];

							for (const auto & body : bodiesInCameraSpace) {
								if (body.tracked) {
									BodiesInWorldSpace.push_back(body * subscriberNode->getTransform());
								}
							}
						}
					}
				}

				return worldBodiesUnmerged;
			}

			//----------
			CombinedBodySet World::combineWorldBodies(WorldBodiesUnmerged worldBodiesUnmerged) const {
				const auto & mergeDistanceThreshold = this->parameters.fusion.mergeDistanceThreshold.get();
				Bodies mergedWorldBodies;

				auto previousFrameCombinedBodies = this->combinedBodies;
				CombinedBodySet newCombinedBodies;

				// 1. Update any bodies which had been seen previously
				// 2. Add any bodies which weren't seen previously (adding them to existing when distance is low)
				// 3. Calculate the merged position
				// as bodies are handled, we remove them from our WorldBodiesUnmerged set
				// 4. Remove inactive bodies

				//--
				//Update bodies seen previously
				//--
				//
				// 1. Go through existing bodies
				// 2. Go through the sources of that body
				// 3. See if it still has this body index
				for (const auto & oldCombinedBodyIterator : previousFrameCombinedBodies) {
					const auto & bodyIndex = oldCombinedBodyIterator.first;

					for (const auto & originalBodiesIterator : oldCombinedBodyIterator.second.originalBodiesWorldSpace) {
						const auto & originalBodyIndex = originalBodiesIterator.second.bodyId;
						const auto & subscriberID = originalBodiesIterator.first;

						auto findSubscriber = worldBodiesUnmerged.find(subscriberID);
						if (findSubscriber != worldBodiesUnmerged.end()) {
							auto & subscriber = findSubscriber->second;
							//find if we have matching body index
							for (auto subscriberBodyIterator = subscriber.begin(); subscriberBodyIterator != subscriber.end(); ) {
								if (subscriberBodyIterator->bodyId == originalBodyIndex) {
									//we've found a body which we'd tagged before
									auto & combinedBody = newCombinedBodies[oldCombinedBodyIterator.first];

									//assign it the new body
									combinedBody.originalBodiesWorldSpace[subscriberID] = *subscriberBodyIterator;

									//remove it from the unassigned set
									subscriberBodyIterator = subscriber.erase(subscriberBodyIterator);
								}
								else {
									subscriberBodyIterator++;
								}
							}
						}
					}
				}
				//
				//--



				//--
				//Add subscriber body to existing combined bodies
				//--
				//
				for (auto & subscriber : worldBodiesUnmerged) {
					auto & bodies = subscriber.second;
					for (auto bodyIterator = bodies.begin(); bodyIterator != bodies.end(); ) {
						// now we have a body belonging to a subscriber

						bool addedToExistingBody = false;

						//check this body against any built bodies so far (this is often empty)
						for (auto & combinedBody : newCombinedBodies) {
							//find the mean of the combinedBody
							auto meanOfExistingBodies = mean(combinedBody.second.originalBodiesWorldSpace, this->getMergeSettings());

							//if the distance between this body and that body is < threshold, add it to the combined body
							auto distance = meanDistance(meanOfExistingBodies, *bodyIterator, true);
							if (distance < this->parameters.fusion.mergeDistanceThreshold) {
								combinedBody.second.originalBodiesWorldSpace[subscriber.first] = *bodyIterator;
								addedToExistingBody = true;
								break;
							}
						}

						if (!addedToExistingBody) {
							static BodyIndex bodyIndex = 0;
							auto & combinedBody = newCombinedBodies[bodyIndex];
							combinedBody.originalBodiesWorldSpace[subscriber.first] = *bodyIterator;
							bodyIndex++;
						}

						//erase this body from unassigned bodies
						bodyIterator = bodies.erase(bodyIterator);
					}
				}
				//
				//--



				// now newCombinedBodies is populated



				//--
				//Calculate the merged position
				//--
				//
				for (auto & newCombinedBody : newCombinedBodies) {
					newCombinedBody.second.combinedBody = mean(newCombinedBody.second.originalBodiesWorldSpace, this->getMergeSettings());
				}
				//
				//--



				//--
				//Remove inactive bodies
				//--
				//
				for (auto newCombinedBodyIterator = newCombinedBodies.begin(); newCombinedBodyIterator != newCombinedBodies.end(); ) {
					if (newCombinedBodyIterator->second.combinedBody.tracked) {
						newCombinedBodyIterator++;
					}
					else {
						newCombinedBodyIterator = newCombinedBodies.erase(newCombinedBodyIterator);
					}

				}
				//
				//--

				return newCombinedBodies;
			}

			//----------
			void World::populateDatabase(Data::Channels::Channel & rootChannel) {
				auto & combined = rootChannel["combined"];
				{
					auto & bodies = combined["bodies"];
					bodies["count"] = (int) this->combinedBodies.size();

					vector<int> indices;
					for (auto body : this->combinedBodies) {
						indices.push_back((int) body.first);
					}
					bodies["indices"] = indices;

					//remove untracked bodies from database
					{
						auto & bodyChannels = bodies["body"].getSubChannels();
						for (auto bodyChannelIterator = bodyChannels.begin(); bodyChannelIterator != bodyChannels.end(); ) {
							const auto bodyIndex = ofToInt(bodyChannelIterator->first);
							if (this->combinedBodies.find(bodyIndex) == this->combinedBodies.end()) {
								bodyChannelIterator = bodyChannels.erase(bodyChannelIterator);
							}
							else {
								bodyChannelIterator++;
							}
						}
					}


					//set data for tracked bodies
					for (auto & combinedBody : this->combinedBodies) {
						auto & bodyChannel = bodies["body"][ofToString(combinedBody.first)];

						auto & body = combinedBody.second.combinedBody;
						bodyChannel["tracked"] = (bool) body.tracked;

						if (!body.tracked) {
							bodyChannel.removeSubChannel("centroid");
							bodyChannel.removeSubChannel("joints");
						}
						else {
							//We use the base of the spine as the centroid
							auto findSpineBase = body.joints.find(JointType::JointType_SpineBase);
							if (findSpineBase != body.joints.end()) {
								bodyChannel["centroid"] = findSpineBase->second.getPosition();
							}
							else {
								bodyChannel.removeSubChannel("centroid");
							}

							auto & jointsChannel = bodyChannel["joints"];
							jointsChannel["count"] = (int) body.joints.size();

							for (auto & joint : body.joints) {
								auto jointName = ofxKinectForWindows2::toString(joint.first);
								auto & jointChannel = jointsChannel[jointName];
								jointChannel["position"] = joint.second.getPosition();
								jointChannel["orientation"] = joint.second.getOrientation().asVec4();
								jointChannel["trackingState"] = joint.second.getTrackingState() == TrackingState::TrackingState_Tracked;
							}
						}
					}
 				}
			}

			//----------
			MergeSettings World::getMergeSettings() const {
				MergeSettings mergeSettings = {
					this->parameters.fusion.crossoverEnabled.get(),
					this->parameters.fusion.crossoverMargin.get()
				};
				return mergeSettings;
			}
		}
	}
}

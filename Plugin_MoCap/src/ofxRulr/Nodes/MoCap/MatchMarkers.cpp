#include "pch_Plugin_MoCap.h"
#include "MatchMarkers.h"

#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
#pragma mark Capture
			//----------
			MatchMarkers::Capture::Capture() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			string MatchMarkers::Capture::getDisplayString() const {
				stringstream ss;
				ss << "MV[R] : " << this->modelViewRotationVector << endl;
				ss << "MV[T] : " << this->modelViewTranslation;
				return ss.str();
			}

			//----------
			void MatchMarkers::Capture::serialize(Json::Value & json) {
				{
					auto & jsonModelViewRotationVector = json["modelViewRotationVector"];
					jsonModelViewRotationVector["x"] = this->modelViewRotationVector.x;
					jsonModelViewRotationVector["y"] = this->modelViewRotationVector.y;
					jsonModelViewRotationVector["z"] = this->modelViewRotationVector.z;
				}

				{
					auto & jsonModelViewTranslation = json["modelViewTranslation"];
					jsonModelViewTranslation["x"] = this->modelViewTranslation.x;
					jsonModelViewTranslation["y"] = this->modelViewTranslation.y;
					jsonModelViewTranslation["z"] = this->modelViewTranslation.z;
				}
			}

			//----------
			void MatchMarkers::Capture::deserialize(const Json::Value & json) {
				{
					const auto & jsonModelViewRotationVector = json["modelViewRotationVector"];
					this->modelViewRotationVector.x = jsonModelViewRotationVector["x"].asDouble();
					this->modelViewRotationVector.y = jsonModelViewRotationVector["y"].asDouble();
					this->modelViewRotationVector.z = jsonModelViewRotationVector["z"].asDouble();
				}

				{
					const auto & jsonModelViewTranslation = json["modelViewTranslation"];
					this->modelViewTranslation.x = jsonModelViewTranslation["x"].asDouble();
					this->modelViewTranslation.y = jsonModelViewTranslation["y"].asDouble();
					this->modelViewTranslation.z = jsonModelViewTranslation["z"].asDouble();
				}
			}

#pragma mark MatchMarkers
			//----------
			MatchMarkers::MatchMarkers() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string MatchMarkers::getTypeName() const {
				return "MoCap::MatchMarkers";
			}

			//----------
			void MatchMarkers::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->manageParameters(this->parameters);
				this->addInput<Body>();
				this->addInput<Item::Camera>();
			}

			//----------
			void MatchMarkers::update() {
				shared_ptr<Body::Description> bodyDescription;
				{
					auto bodyNode = this->getInput<Body>();
					if (bodyNode) {
						bodyDescription = bodyNode->getBodyDescription();
					}
				}

				shared_ptr<CameraDescription> cameraDescription;
				{
					auto cameraNode = this->getInput<Item::Camera>();
					if (cameraNode) {
						cameraDescription = make_shared<CameraDescription>();
						cameraDescription->cameraMatrix = cameraNode->getCameraMatrix();
						cameraDescription->distortionCoefficients = cameraNode->getDistortionCoefficients();
						cameraNode->getExtrinsics(cameraDescription->inverseRotationVector, cameraDescription->inverseTranslation, true);
						cameraDescription->viewProjectionMatrix = cameraNode->getViewInWorldSpace().getViewMatrix() * cameraNode->getViewInWorldSpace().getProjectionMatrix();
					}
				}

				{
					auto lock = unique_lock<mutex>(this->descriptionMutex);
					this->bodyDescription = bodyDescription;
					this->cameraDescription = cameraDescription;
				}

				{
					shared_ptr<Capture> newCapture;
					while (this->newCaptures.tryReceive(newCapture)) {
						this->captures.add(newCapture);
					}
				}
			}

			//----------
			void MatchMarkers::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				this->captures.populateWidgets(inspector);
				inspector->addButton("Add capture", [this]() {
					this->needsTakeCapture.store(true);
				});
			}

			//----------
			void MatchMarkers::serialize(Json::Value & json) {
				this->captures.serialize(json);
			}

			//----------
			void MatchMarkers::deserialize(const Json::Value & json) {
				this->captures.deserialize(json);
			}

			//----------
			void MatchMarkers::processFrame(shared_ptr<FindMarkerCentroidsFrame> incomingFrame) {
				//construct the output frame
				auto outputFrame = make_shared<MatchMarkersFrame>();
				outputFrame->incomingFrame = incomingFrame;

				{
					auto lock = unique_lock<mutex>(this->descriptionMutex);
					outputFrame->bodyDescription = this->bodyDescription;
					if (!outputFrame->bodyDescription) {
						//we can't calculate the frame without a marker body
						return;
					}
					else if (outputFrame->bodyDescription->markerCount == 0) {
						//we can't calculate with an empty marker body
						return;
					}

					outputFrame->cameraDescription = this->cameraDescription;
					if (!outputFrame->cameraDescription) {
						//we can't calculate the frame without a camera
						return;
					}
				}


				//get the distance threshold
				outputFrame->distanceThresholdSquared = this->parameters.trackingDistanceThreshold.get();
				outputFrame->distanceThresholdSquared *= outputFrame->distanceThresholdSquared;

				//process the normal tracking search
				this->processTrackingSearch(outputFrame);

				//try our pre-recorded poses if we failed tracking
				if (outputFrame->result.count < 4) {
					this->processCheckKnownPoses(outputFrame);
				}

				if (this->needsTakeCapture.load()) {
					auto capture = make_shared<Capture>();
					capture->modelViewRotationVector = (cv::Point3d) outputFrame->modelViewRotationVector;
					capture->modelViewTranslation = (cv::Point3d) outputFrame->modelViewTranslation;
					this->newCaptures.send(capture);
					this->needsTakeCapture.store(false);
				}

				this->onNewFrame.notifyListeners(move(outputFrame));
			}

			//----------
			void MatchMarkers::processTrackingSearch(shared_ptr<MatchMarkersFrame> & outputFrame) {
				//combine the camera and body transforms
				cv::composeRT(outputFrame->bodyDescription->rotationVector
					, outputFrame->bodyDescription->translation
					, outputFrame->cameraDescription->inverseRotationVector
					, outputFrame->cameraDescription->inverseTranslation
					, outputFrame->modelViewRotationVector
					, outputFrame->modelViewTranslation);

				//first check the markers are inside the camera image
				//(cvProjectPoints will happily give us weird results for markers outside view, e.g. distortion loop back, behind cam)
				{
					vector<ofVec3f> matrixProjections;
					for (int i = 0; i < bodyDescription->markerCount; i++) {
						const auto & objectSpacePoint = bodyDescription->markers.positions[i];
						const auto worldSpace = objectSpacePoint * bodyDescription->modelTransform;
						const auto projectionSpace = worldSpace * cameraDescription->viewProjectionMatrix;
						if (projectionSpace.z < -1 || projectionSpace.z > 1) {
							//outside of depth clipping range
							continue;
						}
						if (projectionSpace.x < -2.0
							|| projectionSpace.x > 2.0
							|| projectionSpace.y < -2.0
							|| projectionSpace.y > 2.0) {
							//outside of image space (with some allowance for distortion)
							continue;
						}
						outputFrame->search.markerIDs.push_back(bodyDescription->markers.IDs[i]);
						outputFrame->search.objectSpacePoints.emplace_back(move(ofxCv::toCv(objectSpacePoint)));
					}
					outputFrame->search.count = outputFrame->search.markerIDs.size();
				}

				this->processModelViewTransform(outputFrame);
			}

			//----------
			void MatchMarkers::processCheckKnownPoses(shared_ptr<MatchMarkersFrame> & searchFrame) {
				auto captures = this->captures.getSelection();
				for (auto capture : captures) {
					searchFrame->modelViewRotationVector = cv::Mat(capture->modelViewRotationVector);
					searchFrame->modelViewTranslation = cv::Mat(capture->modelViewTranslation);

					this->processModelViewTransform(searchFrame);
					if (searchFrame->result.count >= 4) {
						//this is a result
						return;
					}
				}
			}

			//----------
			bool MatchMarkers::processModelViewTransform(shared_ptr<MatchMarkersFrame> & outputFrame) {
				//project all 3D points from marker body (based on existing/predicted pose) into camera space
				cv::projectPoints(outputFrame->search.objectSpacePoints
					, outputFrame->modelViewRotationVector
					, outputFrame->modelViewTranslation
					, outputFrame->cameraDescription->cameraMatrix
					, outputFrame->cameraDescription->distortionCoefficients
					, outputFrame->search.projectedMarkerImagePoints);

				//clear the result
				outputFrame->result = MatchMarkersFrame::Result();

				for (const auto & centroid : outputFrame->incomingFrame->centroids) {
					map<float, size_t> matchesByDistance; // { distanceSquared, marker index in incoming frame vector}

														  //find all matching markers
					for (size_t i = 0; i < outputFrame->search.count; i++) {
						//find distance
						const auto & markerProjected = outputFrame->search.projectedMarkerImagePoints[i];
						const auto delta = centroid - markerProjected;
						const auto distanceSquared = delta.x * delta.x + delta.y * delta.y;

						if (distanceSquared < outputFrame->distanceThresholdSquared) {
							matchesByDistance.emplace(distanceSquared, i);
						}
					}

					if (matchesByDistance.empty()) {
						//no match found
						continue;
					}

					auto matchIndex = matchesByDistance.begin()->second;
					outputFrame->result.markerListIndicies.push_back(matchIndex);
					outputFrame->result.markerIDs.push_back(outputFrame->search.markerIDs[matchIndex]);
					outputFrame->result.projectedPoints.push_back(outputFrame->search.projectedMarkerImagePoints[matchIndex]);
					outputFrame->result.centroids.push_back(centroid);
					outputFrame->result.objectSpacePoints.push_back(outputFrame->search.objectSpacePoints[matchIndex]);
				}

				outputFrame->result.count = outputFrame->result.markerIDs.size();
			}

		}
	}
}
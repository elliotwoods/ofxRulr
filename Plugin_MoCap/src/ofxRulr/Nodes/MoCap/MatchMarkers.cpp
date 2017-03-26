#include "pch_Plugin_MoCap.h"
#include "MatchMarkers.h"

#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
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
						cameraNode->getExtrinsics(cameraDescription->rotationVector, cameraDescription->translation, true);
					}
				}

				{
					auto lock = unique_lock<mutex>(this->descriptionMutex);
					this->bodyDescription = bodyDescription;
					this->cameraDescription = cameraDescription;
				}
			}

			//----------
			void MatchMarkers::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Search", [this]() {
					this->performSearch();
				}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			void MatchMarkers::performSearch() {
				this->needsFullSearch.store(true);
			}

			//----------
			void MatchMarkers::processFrame(shared_ptr<FindMarkerCentroidsFrame> incomingFrame) {
				//construct the output frame
				auto outputFrame = make_shared<MatchMarkersFrame>();
				outputFrame->incomingFrame = incomingFrame;

				{
					auto lock = unique_lock<mutex>(this->descriptionMutex);
					outputFrame->bodyDescription = this->bodyDescription;
					if (!bodyDescription) {
						//we can't calculate the frame without a marker body
						return;
					}
					else if (bodyDescription->markerCount == 0) {
						//we can't calculate with an empty body
						return;
					}

					outputFrame->cameraDescription = this->cameraDescription;
					if (!cameraDescription) {
						//we can't calculate the frame without a camera body
						return;
					}
				}

				//combine the camera and body transforms
				cv::composeRT(outputFrame->bodyDescription->rotationVector
					, outputFrame->bodyDescription->translation
					, outputFrame->cameraDescription->rotationVector
					, outputFrame->cameraDescription->translation
					, outputFrame->modelViewRotationVector
					, outputFrame->modelViewTranslation);

				//project the points
				outputFrame->objectSpacePoints = ofxCv::toCv(outputFrame->bodyDescription->markers.positions);
				cv::projectPoints(outputFrame->objectSpacePoints
					, outputFrame->modelViewRotationVector
					, outputFrame->modelViewTranslation
					, outputFrame->cameraDescription->cameraMatrix
					, outputFrame->cameraDescription->distortionCoefficients
					, outputFrame->projectedMarkerImagePoints);

				//look for any matches from incoming frame
				outputFrame->distanceThresholdSquared = this->parameters.trackingDistanceThreshold.get();
				outputFrame->distanceThresholdSquared *= outputFrame->distanceThresholdSquared;

				//check if we should do a full search
				if (this->needsFullSearch.load()) {
					this->processFullSearch(outputFrame);
					this->needsFullSearch.store(false);
				}
				else {
					this->processTrackingSearch(outputFrame);
					if (outputFrame->matchCount == 0 && this->parameters.searchWhenTrackingLost.get()) {
						this->processFullSearch(outputFrame);
					}
				}

				this->onNewFrame.notifyListeners(move(outputFrame));
			}

			//----------
			void traverseBranch(MatchMarkersFrame & matchedMarkersFrame, vector<size_t> bodyMarkerIndiciesForTest, map<float, shared_ptr<MatchMarkersFrame>> & results, const float & successErrorThresholdSquared) {
				if (bodyMarkerIndiciesForTest.size() == matchedMarkersFrame.incomingFrame->centroids.size()
					|| bodyMarkerIndiciesForTest.size() == matchedMarkersFrame.bodyDescription->markerCount) {
					//we have a full set

					auto outputFrame = make_shared<MatchMarkersFrame>();
					
					//do the test
					for (const auto & bodyMarkerIndex : bodyMarkerIndiciesForTest) {
						outputFrame->matchedObjectSpacePoints.push_back(ofxCv::toCv(matchedMarkersFrame.bodyDescription->markers.positions[bodyMarkerIndex]));
					}

					outputFrame->matchCount = bodyMarkerIndiciesForTest.size();
					outputFrame->matchedCentroids = matchedMarkersFrame.incomingFrame->centroids;
					outputFrame->matchedCentroids.resize(outputFrame->matchCount); //we might not have used all centroids if there's fewer body points

					//find the state with this presumed arrangement
					cv::solvePnP(outputFrame->matchedObjectSpacePoints
						, outputFrame->matchedCentroids
						, matchedMarkersFrame.cameraDescription->cameraMatrix
						, matchedMarkersFrame.cameraDescription->distortionCoefficients
						, outputFrame->modelViewRotationVector
						, outputFrame->modelViewTranslation
						, false);

					//project the points back
					cv::projectPoints(outputFrame->matchedObjectSpacePoints
						, outputFrame->modelViewRotationVector
						, outputFrame->modelViewTranslation
						, matchedMarkersFrame.cameraDescription->cameraMatrix
						, matchedMarkersFrame.cameraDescription->distortionCoefficients
						, outputFrame->matchedProjectedPoint);

					//calculate the reprojection error
					float sumErrorSquared = 0.0f;
					for (int i = 0; i < bodyMarkerIndiciesForTest.size(); i++) {
						auto delta = outputFrame->matchedProjectedPoint[i] - matchedMarkersFrame.incomingFrame->centroids[i];
						sumErrorSquared += delta.x * delta.x + delta.y * delta.y;
					}

					outputFrame->matchedMarkerListIndex = bodyMarkerIndiciesForTest;
					for (auto & markerIndex : bodyMarkerIndiciesForTest) {
						outputFrame->matchedMarkerID.push_back(matchedMarkersFrame.bodyDescription->markers.IDs[markerIndex]);
					}

					//TODO : what if some of the centroids don't belong to this body

					if (sumErrorSquared < successErrorThresholdSquared) {
						//if this is as good as we want, then throw it up (no, not like that :)
						throw(outputFrame);
					}
					results.emplace(sumErrorSquared, move(outputFrame));
				}
				else {
					//build the branch
					for (int i = 0; i < matchedMarkersFrame.bodyDescription->markerCount; i++) {
						if (find(bodyMarkerIndiciesForTest.begin(), bodyMarkerIndiciesForTest.end(), i) == bodyMarkerIndiciesForTest.end()) {
							//go down this branch
							auto branchMarkerListIndicies = bodyMarkerIndiciesForTest;
							branchMarkerListIndicies.push_back(i);
							traverseBranch(matchedMarkersFrame, branchMarkerListIndicies, results, successErrorThresholdSquared);
						}
					}
				}
			}

			//----------
			void MatchMarkers::processFullSearch(shared_ptr<MatchMarkersFrame> & outputFrame) {
				//performs a brute force nPr search on possible candidates for marker tracking
				map<float, shared_ptr<MatchMarkersFrame>> results;

				if (outputFrame->incomingFrame->centroids.size() < 4
					|| outputFrame->bodyDescription->markerCount < 4) {
					//can't work properly
					return;
				}

				try {
					auto thresholdErrorSquared = this->parameters.searchDistanceThreshold.get();
					thresholdErrorSquared *= thresholdErrorSquared;
					traverseBranch(*outputFrame, vector<size_t>(), results, thresholdErrorSquared);

					if (!results.empty()) {
						cout << "Found a match with " << results.begin()->first << "px error" << endl;
					}
				}
				catch (shared_ptr<MatchMarkersFrame> & result) {
					//we threw early
					results.emplace(0, result);
					cout << "Found a match which meets our search threshold" << endl;
				}

				if (!results.empty()) {
					auto result = results.begin();
					
					//combine the camera and body transforms
					cv::composeRT(result->second->modelViewRotationVector
						, result->second->modelViewTranslation
						, outputFrame->cameraDescription->rotationVector
						, outputFrame->cameraDescription->translation
						, outputFrame->modelViewRotationVector
						, outputFrame->modelViewTranslation);

					outputFrame->matchCount = result->second->matchCount;
					outputFrame->matchedMarkerListIndex = result->second->matchedMarkerListIndex;
					outputFrame->matchedMarkerID = result->second->matchedMarkerID;
					outputFrame->matchedProjectedPoint = result->second->matchedProjectedPoint;
					outputFrame->matchedCentroids = result->second->matchedCentroids;
					outputFrame->matchedObjectSpacePoints = result->second->matchedObjectSpacePoints;
				}
				else {
					cout << "No results found";
				}
			}

			//----------
			void MatchMarkers::processTrackingSearch(shared_ptr<MatchMarkersFrame> & outputFrame) {
				for (const auto & centroid : outputFrame->incomingFrame->centroids) {
					map<float, size_t> matchesByDistance; // { distanceSquared, marker index in incoming frame vector}

					//find all matching markers
					for (size_t i = 0; i < outputFrame->bodyDescription->markerCount; i++) {
						//find distance
						const auto & markerProjected = outputFrame->projectedMarkerImagePoints[i];
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
					outputFrame->matchedMarkerListIndex.push_back(matchIndex);
					outputFrame->matchedMarkerID.push_back(outputFrame->bodyDescription->markers.IDs[matchIndex]);
					outputFrame->matchedProjectedPoint.push_back(outputFrame->projectedMarkerImagePoints[matchIndex]);
					outputFrame->matchedCentroids.push_back(centroid);
					outputFrame->matchedObjectSpacePoints.push_back(outputFrame->objectSpacePoints[matchIndex]);
				}

				outputFrame->matchCount = outputFrame->matchedMarkerListIndex.size();
			}
		}
	}
}
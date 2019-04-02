#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			//----------
			FitPointsToLines::FitPointsToLines() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string FitPointsToLines::getTypeName() const {
				return "LSS::FitPointsToLines";
			}

			//----------
			void FitPointsToLines::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Item::Camera>();
				this->addInput<Item::Projector>();
				this->addInput<Procedure::Scan::Graycode>();

				this->manageParameters(this->parameters);
			}

			//----------
			void FitPointsToLines::drawWorldStage() {
				if (this->parameters.preview.unclassifiedPixels) {
					this->previewUnclassifiedPixels.draw();
				}

				if (this->parameters.preview.lines) {
					ofPushStyle();
					{
						this->previewLines.draw();
					}
					ofPopStyle();
				}

				if (this->parameters.preview.debug) {
					if (int(ofGetElapsedTimef()) % 2 == 0) {
						ofPushStyle();
						{
							ofSetSphereResolution(4);
							ofNoFill();
							for (auto debugPoint : this->debugPoints) {
								ofDrawSphere(debugPoint, 0.02);
							}
						}
						ofPopStyle();
					}
					this->debugRay.draw();
				}
			}

			//----------
			void FitPointsToLines::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;

				inspector->addButton("Gather projector pixels", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Gather projector pixels");
						this->gatherProjectorPixels();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Triangulate", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Triangulate");
						this->triangulate();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Fit", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Fit points to lines");
						this->fit();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			void FitPointsToLines::gatherProjectorPixels() {
				this->throwIfMissingAnyConnection();

				auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();

				if (!graycodeNode->hasData()) {
					throw(ofxRulr::Exception("No scan data available"));
				}
				const auto & dataSet = graycodeNode->getDataSet();

				map<uint32_t, shared_ptr<ProjectorPixel>> unclassifiedPixels;

				// gather projector pixels
				for (const auto & pixel : dataSet) {
					// our own distance threshold check
					if (pixel.distance < this->parameters.gatherProjectorPixels.distanceThreshold) {
						continue;
					}

					auto findExistingPixel = unclassifiedPixels.find(pixel.projector);
					if (findExistingPixel != unclassifiedPixels.end()) {
						// we already have it
						findExistingPixel->second->cameraFinds.push_back(pixel.getCameraXY());
					}
					else {
						// make a new one
						auto newProjectorPixel = make_shared<ProjectorPixel>();
						newProjectorPixel->cameraFinds.push_back(pixel.getCameraXY());
						newProjectorPixel->projector = pixel.getProjectorXY();
						unclassifiedPixels.emplace(pixel.projector, move(newProjectorPixel));
					}
				}

				this->unclassifiedPixels = unclassifiedPixels;
			}

			//----------
			template<typename T>
			T mean(const vector<T> & list) {
				T accumulator;
				for (const auto & element : list) {
					accumulator = accumulator + element;
				}
				return accumulator / list.size();
			}

			template<typename T>
			float maxDistance(const vector<T> & list, const T & position) {
				float maxDistance2 = 0;
				for (const auto & element : list) {
					auto distance2 = element.squareDistance(position);
					if (distance2 > maxDistance2) {
						distance2 = maxDistance2;
					}
				}

				return sqrt(maxDistance2);
			}

			template<typename T>
			vector<T> trimFarthest(const vector<T> & list, const T & position) {
				float maxDistance2 = 0;
				vector<T>::const_iterator farthestPositionIt;
				bool found = false;

				for (auto it = list.begin(); it != list.end(); it++) {
					const auto & element = *it;
					auto distance2 = element.squareDistance(position);
					if (distance2 > maxDistance2) {
						distance2 = maxDistance2;
						farthestPositionIt = it;
						found = true;
					}
				}

				if (found) {
					auto newList = list;
					newList.erase(farthestPositionIt);
					return newList;
				}
				else {
					return list;
				}
			}

			void FitPointsToLines::triangulate() {
				this->throwIfMissingAnyConnection();

				auto cameraNode = this->getInput<Item::Camera>();
				auto projectorNode = this->getInput<Item::Projector>();
				
				auto cameraMatrix = cameraNode->getCameraMatrix();
				auto distortionCoefficients = cameraNode->getDistortionCoefficients();

				auto cameraView = cameraNode->getViewInWorldSpace();
				auto projectorView = projectorNode->getViewInWorldSpace();

				//we will perform this undistortion ourselves
				cameraView.distortion.clear();

				ofMesh previewUnclassifiedPixels;
				previewUnclassifiedPixels.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

				for (auto & projectorPixelIt : this->unclassifiedPixels) {
					auto & projectorPixel = * projectorPixelIt.second;

					// get the average camera position
					{
						auto selectedCameraPixels = projectorPixel.cameraFinds;

						// trim outliers
						auto meanCameraPosition = mean(selectedCameraPixels);
						while (maxDistance(selectedCameraPixels, meanCameraPosition) > this->parameters.triangulate.maxCameraDeviation) {
							//trim the pixel which is furthest from the mean
							selectedCameraPixels = trimFarthest(selectedCameraPixels, meanCameraPosition);
							meanCameraPosition = mean(selectedCameraPixels);
						}

						projectorPixel.camera = meanCameraPosition;
					}
					
					projectorPixel.cameraUndistorted = ofxCv::toOf(ofxCv::undistortPixelCoordinates(vector<cv::Point2f>(1, ofxCv::toCv(projectorPixel.camera))
						, cameraMatrix
						, distortionCoefficients).front());

					auto cameraPixelRay = cameraView.castPixel(projectorPixel.cameraUndistorted);
					auto projectorPixelRay = projectorView.castPixel(projectorPixel.projector);

					auto intersection = cameraPixelRay.intersect(projectorPixelRay);
					projectorPixel.world = intersection.getMidpoint();
					projectorPixel.intersectionLength = intersection.getLength();

					//preview
					{
						previewUnclassifiedPixels.addVertex(projectorPixel.world);
					}
				}

				this->previewUnclassifiedPixels = previewUnclassifiedPixels;
			}

			//----------
			ofxRay::Ray FitPointsToLines::fitRayToProjectorPixels(vector<shared_ptr<ProjectorPixel>> projectorPixels) {
				vector<ofVec3f> dataPoints;
				for (auto projectorPixel : projectorPixels) {
					dataPoints.push_back(projectorPixel->world);
				}
				auto meanPosition = mean(dataPoints);

				cv::Vec6f line;
				cv::fitLine(ofxCv::toCv(dataPoints)
					, line
					, CV_DIST_HUBER
					, 0
					, this->parameters.search.head.deviationThreshold
					, 0.01);

				ofxRay::Ray ray;

				ray.t.x = line(0);
				ray.t.y = line(1);
				ray.t.z = line(2);
				ray.s.x = line(3);
				ray.s.y = line(4);
				ray.s.z = line(5);

				ray.s = ray.closestPointOnRayTo(meanPosition);

				this->debugPoints = dataPoints;
				this->debugRay = ray;

				return ray;
			}

			void FitPointsToLines::fit() {
				vector<shared_ptr<Line>> lines;

				for (auto & projectorPixelIt : this->unclassifiedPixels) {
					auto & projectorPixel = *projectorPixelIt.second;
					if (projectorPixel.unavailable) {
						// This pixel is already used
						continue;
					}

					// make a line object
					auto newLine = make_shared<Line>();


					// build up a set of matching pixels
					for (auto & projectorPixelIt2 : this->unclassifiedPixels) {
						auto & projectorPixel2 = projectorPixelIt2.second;
						if (projectorPixel2->unavailable) {
							continue;
						}
						if (projectorPixel2->world.distance(projectorPixel.world) < this->parameters.search.head.sizeM
							&& projectorPixel2->projector.distance(projectorPixel.projector) < this->parameters.search.head.sizePx) {
							newLine->projectorPixels.push_back(projectorPixel2);
						}
					}

					// test count is above threshold for a new line
					if (newLine->projectorPixels.size() < this->parameters.search.head.minimumCount) {
						continue;
					}

					// fit a 3D line
					ofxRay::Ray ray = this->fitRayToProjectorPixels(newLine->projectorPixels);

					// count deviations
					size_t deviationCount = 0;
					vector<shared_ptr<ProjectorPixel>> goodPixels;
					for (auto projectorPixel : newLine->projectorPixels) {
						auto deviationFromLine = ray.distanceTo(projectorPixel->world);
						if (deviationFromLine > this->parameters.search.head.deviationThreshold) {
							deviationCount++;
						}
						else {
							goodPixels.push_back(projectorPixel);
						}
					}

					// test count is above threshold for a new line
					if (newLine->projectorPixels.size() - deviationCount < this->parameters.search.head.minimumCount) {
						continue;
					}

					// take the pixels
					newLine->projectorPixels = goodPixels;

					// re-fit to the good pixels
					ray = this->fitRayToProjectorPixels(goodPixels);

					auto walkDistance = this->parameters.search.walk.distance;
					vector<float> directions{ -1, +1 };
					vector<ofVec3f> lineEnds;
					for (auto direction : directions) {
						ofVec3f walkPosition = ray.s;
						bool enoughPieces;

						while (true) {
							//find pixels in this region
							vector<shared_ptr<ProjectorPixel>> addProjectorPixels;
							for (const auto & projectorPixel2It : this->unclassifiedPixels) {
								auto projectorPixel2 = projectorPixel2It.second;
								if (projectorPixel2->unavailable) {
									continue;
								}
								if (projectorPixel2->world.distance(walkPosition) < this->parameters.search.walk.sizeM) {
									addProjectorPixels.push_back(projectorPixel2);
								}
							}

							//check we have enough
							if (addProjectorPixels.size() < this->parameters.search.walk.minimumCount) {
								break;
							}

							//update the line definition
							for (auto pixelToAdd : addProjectorPixels) {
								newLine->projectorPixels.push_back(pixelToAdd);
								debugPoints.push_back(pixelToAdd->world);
							}

							//re-fit the ray
							ray = this->fitRayToProjectorPixels(newLine->projectorPixels);

							//walk forwards
							walkPosition += ray.t * walkDistance * direction;
						}
						lineEnds.push_back(walkPosition);
					}
					newLine->startWorld = lineEnds[0];
					newLine->endWorld = lineEnds[1];

					//check the tests
					{
						if (newLine->startWorld.distance(newLine->endWorld) < this->parameters.search.test.minimumLength) {
							continue;
						}

						int deviantCount = 0;
						for (auto projectorPixel2 : newLine->projectorPixels) {
							auto deviation = ray.distanceTo(projectorPixel2->world);
							if (deviation > this->parameters.search.test.deviationThreshold) {
								deviantCount++;
							}
						}
						float deviantRatio = (float) deviantCount / (float) newLine->projectorPixels.size();
						if (deviantRatio > this->parameters.search.test.deviantPopulationLimit) {
							continue;
						}
					}
					
					
					//take and mark projector pixels
					vector<shared_ptr<ProjectorPixel>> uniqueProjectorPixels;
					for (auto projectorPixel : newLine->projectorPixels) {
						if (projectorPixel->unavailable) {
							continue;
						}
						else {
							uniqueProjectorPixels.push_back(projectorPixel);
							projectorPixel->unavailable = true;
						}
					}
					newLine->projectorPixels = uniqueProjectorPixels;

					lines.push_back(newLine);
					cout << "."; // some progress indication
				}
				cout << endl;

				this->lines = lines;


				// build preview
				{
					ofMesh previewLines;
					previewLines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

					for (auto line : lines) {
						previewLines.addVertex(line->startWorld);
						previewLines.addVertex(line->endWorld);

						auto color = ofColor(200, 100, 100);
						color.setHueAngle(ofRandom(360.0));
						previewLines.addColor(color);
						previewLines.addColor(color);
					}
					this->previewLines = previewLines;
				}
			}
		}
	}
}
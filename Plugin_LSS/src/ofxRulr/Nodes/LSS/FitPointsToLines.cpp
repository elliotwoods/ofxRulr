#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
#pragma mark Line
			//----------
			FitPointsToLines::Line::Line() {
				this->color = ofColor(200, 100, 100);
				this->color.setHueAngle(ofRandom(360.0));
			}

#pragma mark FitPointsToLines
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
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Item::Camera>();
				this->addInput<Item::Projector>();
				this->addInput<Procedure::Scan::Graycode>();
				auto videoOutput = this->addInput<System::VideoOutput>();

				this->manageParameters(this->parameters);

				{
					auto panel = ofxCvGui::Panels::makeBaseDraws(this->previewInProjector, "Lines in projector");
					this->panel = panel;
				}

				{
					videoOutput->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						videoOutput->onDrawOutput.addListener([this](ofRectangle & outputRectangle) {
							this->drawOnProjector();
						}, this);
					};
					videoOutput->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						if (videoOutput) {
							videoOutput->onDrawOutput.removeListeners(this);
						}
					};
				}
			}

			//----------
			void FitPointsToLines::update() {
				if (this->previewDirty) {
					this->rebuildPreview();
				}
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

				if (this->parameters.preview.classifiedPixels) {
					this->previewClassifiedPixels.draw();
				}

				if (this->parameters.preview.debug) {
					if (int(ofGetElapsedTimef()) % 2 == 0) {
						ofPushStyle();
						{
							ofSetSphereResolution(2);
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
			ofxCvGui::PanelPtr FitPointsToLines::getPanel() {
				return this->panel;
			}

			//----------
			void FitPointsToLines::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;

				inspector->addButton("Clear", [this]() {
					this->lines.clear();
					this->unclassifiedPixels.clear();
					this->previewDirty = true;
				});

				inspector->addButton("Calibrate", [this]() {
					try {
						this->calibrate();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, OF_KEY_RETURN)->setHeight(100.0f);

				inspector->addButton("Gather projector pixels", [this]() {
					try {
						this->gatherProjectorPixels();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addLiveValue<size_t>("Unclassified projector pixels", [this]() {
					return this->unclassifiedPixels.size();
				});

				inspector->addButton("Triangulate", [this]() {
					try {
						this->triangulate();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Fit", [this]() {
					try {
						this->fit();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addLiveValue<size_t>("Found line count", [this]() {
					return this->lines.size();
				});

				inspector->addButton("Project to 2D", [this]() {
					try {
						this->projectTo2D();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Export JSON", [this]() {
					try {
						this->exportData();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			void FitPointsToLines::calibrate() {
				Utils::ScopedProcess scopedProcess("Calibrate");
				this->gatherProjectorPixels();
				this->triangulate();
				this->fit();
				this->projectTo2D();
				this->exportData();
				scopedProcess.end();
			}

			//----------
			void FitPointsToLines::gatherProjectorPixels() {
				Utils::ScopedProcess scopedProcess("Gather projector pixels");
				this->throwIfMissingAConnection<Procedure::Scan::Graycode>();

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
				this->previewDirty = true;

				scopedProcess.end();
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
					auto distance2 = glm::distance2(element, position);
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
					auto distance2 = glm::distance2(element, position);
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
				Utils::ScopedProcess scopedProcess("Triangulate");
				this->throwIfMissingAConnection<Item::Camera>();
				this->throwIfMissingAConnection<Item::Projector>();

				auto cameraNode = this->getInput<Item::Camera>();
				auto projectorNode = this->getInput<Item::Projector>();
				
				auto cameraMatrix = cameraNode->getCameraMatrix();
				auto distortionCoefficients = cameraNode->getDistortionCoefficients();

				auto cameraView = cameraNode->getViewInWorldSpace();
				auto projectorView = projectorNode->getViewInWorldSpace();

				//we will perform this undistortion ourselves
				cameraView.distortion.clear();

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

					//tests
					projectorPixel.triangulationFailed = false;
					if (intersection.getLength() > this->parameters.triangulate.maxIntersectionLength) {
						projectorPixel.triangulationFailed = true;
					}
					auto cameraReprojection = cameraView.getScreenCoordinateOfWorldPosition(projectorPixel.world);
					auto cameraReprojectionError = glm::distance((glm::vec2) cameraReprojection, projectorPixel.cameraUndistorted);
					if (cameraReprojectionError > this->parameters.triangulate.maxCameraDeviation) {
						projectorPixel.triangulationFailed = true;
					}
				}

				this->previewDirty = true;
				scopedProcess.end();
			}

			//----------
			ofxRay::Ray FitPointsToLines::fitRayToProjectorPixels(vector<shared_ptr<ProjectorPixel>> projectorPixels) {
				vector<glm::vec3> dataPoints;
				for (auto projectorPixel : projectorPixels) {
					dataPoints.push_back(projectorPixel->world);
				}
				auto meanPosition = mean(dataPoints);

				cv::Vec6f line;
				cv::fitLine(ofxCv::toCv(dataPoints)
					, line
					, cv::DIST_HUBER
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
				Utils::ScopedProcess scopedProcess("Fit points to lines");
				vector<shared_ptr<Line>> lines;

				this->throwIfMissingAConnection<Item::Camera>();
				auto cameraPosition = this->getInput<Item::Camera>()->getPosition();

				size_t countProgress = 0;
				for (auto & projectorPixelIt : this->unclassifiedPixels) {
					countProgress++;

					auto & projectorPixel = *projectorPixelIt.second;
					if (projectorPixel.unavailable || projectorPixel.triangulationFailed) {
						// This pixel is already used or not suitable
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
						if (glm::distance(projectorPixel2->world, projectorPixel.world) < this->parameters.search.head.sizeM
							&& glm::distance(projectorPixel2->projector, projectorPixel.projector) < this->parameters.search.head.sizePx) {
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

					auto walkDistance = this->parameters.search.walk.length;
					vector<float> directions{ -1, +1 };
					vector<glm::vec3> lineEnds;
					for (auto direction : directions) {
						glm::vec3 walkPosition = ray.s;
						while (true) {
							//find pixels in this region
							vector<shared_ptr<ProjectorPixel>> addProjectorPixels;
							for (const auto & projectorPixel2It : this->unclassifiedPixels) {
								auto projectorPixel2 = projectorPixel2It.second;
								if (projectorPixel2->unavailable) {
									continue;
								}
								auto pointProjectedOnLine = ray.closestPointOnRayTo(projectorPixel2->world);

								auto rayToProjectorPixelDeviation = projectorPixel2->world - ray.closestPointOnRayTo(projectorPixel2->world);

								auto viewDirection = projectorPixel2->world - cameraPosition;
								viewDirection = glm::normalize(viewDirection);

								auto deviationInViewDirection = abs(glm::dot(viewDirection, rayToProjectorPixelDeviation));
								auto deviationOrthogonalToViewDirection = abs(glm::length(glm::cross(viewDirection, rayToProjectorPixelDeviation)));

								if (deviationOrthogonalToViewDirection < this->parameters.search.walk.width / 2.0f
									&& deviationInViewDirection < this->parameters.search.walk.depth / 2.0f
									&& glm::distance(pointProjectedOnLine, walkPosition) < this->parameters.search.walk.length / 2.0f) {
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
							walkPosition += ray.t * walkDistance.get() * direction;
						}
						lineEnds.push_back(walkPosition);
					}
					newLine->startWorld = lineEnds[0];
					newLine->endWorld = lineEnds[1];

					//take average of closest points
					{
						auto pointNearStart = this->findMeanClosestWorld(newLine->projectorPixels, newLine->startWorld);
						auto pointNearEnd = this->findMeanClosestWorld(newLine->projectorPixels, newLine->endWorld);

						newLine->startWorld = ray.closestPointOnRayTo(pointNearStart);
						newLine->endWorld= ray.closestPointOnRayTo(pointNearEnd);
					}

					//check the tests
					{
						if (glm::distance(newLine->startWorld, newLine->endWorld) < this->parameters.search.test.minimumLength) {
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
					{
						auto progressPercent = countProgress * 100 / this->unclassifiedPixels.size();
						Utils::ScopedProcess scopedProcess2("Adding line " + ofToString(lines.size()) + " (" + ofToString(progressPercent) + "% progress)", false);
					}

					if (lines.size() >= this->parameters.search.test.maxLineCount) {
						ofSystemAlertDialog("Too many lines [" + ofToString(lines.size()) + "]");
						break;
					}
				}

				this->lines = lines;

				// Strip classified pixels
				{
					map<uint32_t, shared_ptr<ProjectorPixel>> unclassifiedProjectorPixels;
					for (auto projectorPixel : this->unclassifiedPixels) {
						if (!projectorPixel.second->unavailable) {
							unclassifiedProjectorPixels.emplace(projectorPixel.first, projectorPixel.second);
						}
					}
					this->unclassifiedPixels = unclassifiedProjectorPixels;
				}

				this->previewDirty = true;
				scopedProcess.end();
			}

			glm::vec3 to3(const glm::vec2& vec2)
			{
				return {
					vec2.x
					, vec2.y
					, 0.0f
				};
			}

			//----------
			void FitPointsToLines::projectTo2D() {
				Utils::ScopedProcess scopedProcess("Project to 2D");
				auto fitRayTo2DPoints = [this](const vector<glm::vec2> & points) {
					cv::Vec4f line;
					cv::fitLine(ofxCv::toCv(points)
						, line
						, cv::DIST_HUBER
						, 0
						, this->parameters.projectTo2D.deviationThreshold
						, 0.01);

					ofxRay::Ray ray;
					ray.s = glm::vec3(line(2), line(3), 0.0f);
					ray.t = glm::vec3(line(0), line(1), 0.0f);

					return ray;
				};

				for (auto line : this->lines) {
					//create a 2D ray
					vector<glm::vec2> projectorPixelPositions;
					for (auto projectorPixel : line->projectorPixels) {
						projectorPixelPositions.push_back(projectorPixel->projector);
					}
					auto rayInProjectionImage = fitRayTo2DPoints(projectorPixelPositions);

					//find closest points to each end
					auto meanProjectorPixelStart = this->findMeanClosestProjected(line->projectorPixels, line->startWorld);
					auto meanProjectorPixelEnd = this->findMeanClosestProjected(line->projectorPixels, line->endWorld);

					//project onto ray
					auto projectedPixelStart = rayInProjectionImage.closestPointOnRayTo(to3(meanProjectorPixelStart));
					auto projectedPixelEnd = rayInProjectionImage.closestPointOnRayTo(to3(meanProjectorPixelEnd));

					line->startProjector = (glm::vec2)projectedPixelStart;
					line->endProjector = (glm::vec2)projectedPixelEnd;
				}

				this->previewDirty = true;
				scopedProcess.end();
			}

			//----------
			template<typename T>
			void serializeVector(nlohmann::json & json, const T & vector) {
				for (int i = 0; i < sizeof(vector) / sizeof(float); i++) {
					json[i] = vector[i];
				}
			}

			void FitPointsToLines::exportData() {
				Utils::ScopedProcess scopedProcess("Export data");

				nlohmann::json jsonOuter;
				auto & json = jsonOuter["lines"];

				int lineIndex = 0;
				for (auto line : this->lines) {
					nlohmann::json jsonLine;
					
					serializeVector(jsonLine["startWorld"], line->startWorld);
					serializeVector(jsonLine["endWorld"], line->endWorld);

					serializeVector(jsonLine["startProjector"], line->startProjector);
					serializeVector(jsonLine["endProjector"], line->endProjector);

					json.push_back(jsonLine);
				}


				ofFile file;
				file.open(this->getName() + "-lines.json", ofFile::WriteOnly, false);
				file << jsonOuter;

				scopedProcess.end();
			}

			//----------
			vector<shared_ptr<FitPointsToLines::ProjectorPixel>> FitPointsToLines::findNClosestPixels(const vector<shared_ptr<ProjectorPixel>> projectorPixels, const glm::vec3 & position, int count) {
				// put all pixels into a map
				map<float, shared_ptr<ProjectorPixel>> projectorPixelsByDistance;
				for (auto projectorPixel : projectorPixels) {
					auto distance = glm::distance2(position, projectorPixel->world);
					projectorPixelsByDistance.emplace(distance, projectorPixel);
				}

				// copy first N from map into vector
				auto it = projectorPixelsByDistance.begin();
				vector<shared_ptr<ProjectorPixel>> results;
				for (int i = 0; i < count; i++) {
					//break if we get to end of map
					if (it == projectorPixelsByDistance.end()) {
						break;
					}

					results.push_back(it->second);
					it++;
				}

				return results;
			};

			//----------
			glm::vec2 FitPointsToLines::findMeanClosestProjected(const vector<shared_ptr<ProjectorPixel>> & projectorPixels, const glm::vec3 & worldPosition) {
				auto closestPixels = this->findNClosestPixels(projectorPixels
					, worldPosition
					, this->parameters.projectTo2D.closePixelCount);
				vector<glm::vec2> projectorPixelPositions;
				for (auto closePixel : closestPixels) {
					projectorPixelPositions.push_back(closePixel->projector);
				}
				auto meanProjectorPixel = mean(projectorPixelPositions);
				return meanProjectorPixel;
			};

			//----------
			glm::vec3 FitPointsToLines::findMeanClosestWorld(const vector<shared_ptr<ProjectorPixel>> & projectorPixels, const glm::vec3 & worldPosition) {
				auto closestPixels = this->findNClosestPixels(projectorPixels
					, worldPosition
					, this->parameters.projectTo2D.closePixelCount);
				vector<glm::vec3> worldPositions;
				for (auto closePixel : closestPixels) {
					worldPositions.push_back(closePixel->world);
				}
				auto meanWorldPosition = mean(worldPositions);
				return meanWorldPosition;
			};

			//----------
			void FitPointsToLines::rebuildPreview() {
				Utils::ScopedProcess scopedProcess("Rebuild preview");

				this->previewDirty = false;

				// unclassified pixels
				{
					ofMesh previewUnclassifiedPixels;
					previewUnclassifiedPixels.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

					for(auto unclassifiedPixel : this->unclassifiedPixels) {
						previewUnclassifiedPixels.addVertex(unclassifiedPixel.second->world);

						if (unclassifiedPixel.second->triangulationFailed) {
							previewUnclassifiedPixels.addColor(ofColor(30));
						}
						else {
							previewUnclassifiedPixels.addColor(ofColor(255));
						}
					}

					swap(this->previewUnclassifiedPixels, previewUnclassifiedPixels);
				}

				// classified pixels
				{
					ofMesh previewClassifiedPixels;
					previewClassifiedPixels.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

					for (auto line : this->lines) {
						for (auto projectorPixel : line->projectorPixels) {
							previewClassifiedPixels.addVertex(projectorPixel->world);
							previewClassifiedPixels.addColor(line->color);
						}
					}

					swap(this->previewClassifiedPixels, previewClassifiedPixels);
				}

				// lines in 3D
				{
					ofMesh previewLines;
					previewLines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

					for (auto line : this->lines) {
						previewLines.addVertex(line->startWorld);
						previewLines.addVertex(line->endWorld);

						previewLines.addColor(line->color);
						previewLines.addColor(line->color);
					}

					swap(this->previewLines, previewLines);
				}

				// preview in projector
				{
					auto projectorNode = this->getInput<Item::Projector>();
					if (projectorNode) {
						//allocate
						{
							ofFbo::Settings settings;
							{
								settings.width = projectorNode->getWidth();
								settings.height = projectorNode->getHeight();
							}
							this->previewInProjector.allocate(settings);
						}

						this->previewInProjector.begin();
						{
							ofPushStyle();
							{
								for (auto line : this->lines) {
									ofSetColor(line->color);
									ofDrawLine(line->startProjector, line->endProjector);
								}
							}
							ofPopStyle();
						}
						this->previewInProjector.end();
					}
				}

				scopedProcess.end();
			}

			//----------
			void FitPointsToLines::drawOnProjector() {
				if (this->parameters.preview.onProjector && this->previewInProjector.isAllocated()) {
					this->previewInProjector.draw(0, 0);
				}
			}
		}
	}
}
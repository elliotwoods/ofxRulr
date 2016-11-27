#include "pch_RulrNodes.h"
#include "AddScan.h"

#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					//----------
					AddScan::AddScan() {
						RULR_NODE_INIT_LISTENER;
					}

					//----------
					string AddScan::getTypeName() const {
						return "Calibrate::ProCamSolver::AddScan";
					}

					//----------
					void AddScan::init() {
						RULR_NODE_INSPECTOR_LISTENER;
						RULR_NODE_SERIALIZATION_LISTENERS;
						RULR_NODE_DRAW_WORLD_LISTENER;

						this->addInput<Scan::Graycode>();
						this->addInput<Item::Projector>();
					}

					//----------
					void AddScan::drawWorld() {
						this->previewRays.draw();
					}

					//----------
					void AddScan::serialize(Json::Value & json) {
						Utils::Serializable::serialize(json, this->parameters);
					}

					//----------
					void AddScan::deserialize(const Json::Value & json) {
						Utils::Serializable::deserialize(json, this->parameters);
					}

					//----------
					void AddScan::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
						auto inspector = inspectArgs.inspector;
						
						{
							inspector->addTitle("Fit parameters", ofxCvGui::Widgets::Title::H2);
							inspector->addSlider(this->parameters.includeForFitRatio);
							inspector->addButton("Build preview rays", [this]() {
								try {
									this->buildPreviewRays();
								}
								RULR_CATCH_ALL_TO_ALERT;
							});
							inspector->addLiveValue<size_t>("Preview ray pair count", [this]() {
								return this->previewRays.getNumVertices() / (2 * 2);
							});
							inspector->addButton("Clear preview rays", [this]() {
								this->previewRays.clear();
							});
						}
					}

					//----------
					vector<AddScan::DataPoint> AddScan::getFitPoints() const {
						Utils::ScopedProcess scopedProcess("Get fit points");

						this->throwIfMissingAConnection<Scan::Graycode>();
						auto graycodeNode = this->getInput<Scan::Graycode>();

						graycodeNode->throwIfMissingAConnection<Item::Camera>();
						auto cameraNode = graycodeNode->getInput<Item::Camera>();

						this->throwIfMissingAConnection<Item::Projector>();
						auto projectorNode = this->getInput<Item::Projector>();

						auto cameraView = cameraNode->getViewInWorldSpace();
						auto projectorView = projectorNode->getViewInWorldSpace();

						auto & scanDataSet = graycodeNode->getDataSet();
						if (!scanDataSet.getHasData()) {
							throw(ofxRulr::Exception("Scan has no data"));
						}

						//start with set of data
						typedef map<uint32_t, ofxGraycode::DataSet::const_iterator> PointSet;
						PointSet activeCameraPixels;
						{
							Utils::ScopedProcess scopedProcessActivePixels("Get all active pixels", false);
							for (ofxGraycode::DataSet::const_iterator pixelIt = scanDataSet.begin(); pixelIt != scanDataSet.end(); pixelIt.operator++()) {
								auto & pixel = pixelIt.operator->();
								if (pixel.active) {
									activeCameraPixels[pixel.camera] = pixelIt;
								}
							}
						}


						//split the data into a 4x4 grid (quad tree approach would be nicer if we have more time)
						//and separate into bins of distance (higher is best)
						const uint32_t GRID_RES = 4;

						//array (per grid square) of map<distance, vector<points>> 
						map<uint8_t, vector<ofxGraycode::DataSet::const_iterator>> pixelsByDistancePerGridSquare[GRID_RES * GRID_RES];

						{
							Utils::ScopedProcess scopedProcessActivePixels("Split scan points into grid and distance bins", false);
							uint32_t gridCellWidth = cameraNode->getWidth() / GRID_RES;
							uint32_t gridCellHeight = cameraNode->getHeight() / GRID_RES;

							for (auto & point : activeCameraPixels) {
								auto & pixel = point.second.operator->();
								auto cameraXY = pixel.getCameraXY();

								int gridIndex = ((uint32_t)cameraXY.x / gridCellWidth) + ((uint32_t)cameraXY.y / gridCellHeight) * GRID_RES;
								pixelsByDistancePerGridSquare[gridIndex][pixel.distance].push_back(point.second);
							}
						}

						
						vector<AddScan::DataPoint> dataPoints;
						size_t targetSize = (float)activeCameraPixels.size() * this->parameters.includeForFitRatio;

						//accumulate fit points and remove as we go along
						{
							Utils::ScopedProcess scopedProcessAccumulatePoints("Split scan points into grid and distance bins", false);
							size_t gridSquare = 0;
							while (dataPoints.size() < targetSize) {
								auto & pointsForThisSquare = pixelsByDistancePerGridSquare[gridSquare];

								//if no bins available, continue to next square
								//remove best bin if empty
								//if no bins available, continue to next square
								//find best point
								//remove it from bin

								{
									if (pointsForThisSquare.empty()) {
										goto continueToNextSquare;
									}
									auto bestPointsIt = pointsForThisSquare.rbegin();
									while (bestPointsIt->second.empty()) {
										pointsForThisSquare.erase(bestPointsIt->first);
										if (pointsForThisSquare.empty()) {
											goto continueToNextSquare;
										}
										bestPointsIt = pointsForThisSquare.rbegin();
									}
									auto point = bestPointsIt->second.back().operator->();
									DataPoint dataPoint{
										cameraView.pixelToCoordinate(point.getCameraXY()),
										projectorView.pixelToCoordinate(point.getProjectorXY()),
										point.median
									};
									dataPoints.push_back(dataPoint);

									bestPointsIt->second.pop_back();
								}
							continueToNextSquare:
								gridSquare++;
								gridSquare %= (GRID_RES * GRID_RES);
							}
						}
						
						scopedProcess.end();

						return dataPoints;
					}

					//----------
					shared_ptr<ofxRulr::Nodes::Base> AddScan::getNode() {
						return this->getInput<Scan::Graycode>();
					}

					//----------
					void AddScan::buildPreviewRays() {
						this->previewRays.clear();
						auto fitPoints = this->getFitPoints();

						//we should have already thrown if camera or projector is missing
						auto cameraNode = this->getInput<Scan::Graycode>()->getInput<Item::Camera>();
						auto projectorNode = this->getInput<Item::Projector>();
						
						auto cameraView = cameraNode->getViewInWorldSpace();
						auto projectorView = projectorNode->getViewInWorldSpace();

						for (auto & fitPoint : fitPoints) {
							auto cameraRay = cameraView.castCoordinate(fitPoint.camera);
							auto projectorRay = projectorView.castCoordinate(fitPoint.projector);

							this->previewRays.addVertices({ cameraRay.getStart()
								, cameraRay.getMidpoint()
								, projectorRay.getStart()
								, projectorRay.getMidpoint()
							});
							
							ofColor color(fitPoint.median);

							this->previewRays.addColors({ color
							, color
							, color
							, color });
						}

						previewRays.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
					}
				}
			}
		}
	}
}
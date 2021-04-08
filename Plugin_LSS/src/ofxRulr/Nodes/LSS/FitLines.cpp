#include "pch_Plugin_LSS.h"

using json = nlohmann::json;

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			//----------
			FitLines::FitLines() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string FitLines::getTypeName() const {
				return "LSS::FitLines";
			}

			//----------
			void FitLines::init() {
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<World>();
				this->manageParameters(this->parameters);
			}

			//----------
			void FitLines::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;
				inspector->addButton("Fit lines", [this]() {
					try {
						this->fit();
					}
					RULR_CATCH_ALL_TO_ALERT;
				})->setHeight(100.0f);
				inspector->addButton("Pick lines", [this]() {
					try {
						this->pick();
					}
					RULR_CATCH_ALL_TO_ALERT;
				})->setHeight(100.0f);
				inspector->addButton("Full auto", [this]() {
					try {
						this->fullAuto();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, OF_KEY_RETURN)->setHeight(100.0f);
				inspector->addButton("Save lines.json", [this]() {
					try {
						this->saveResults();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			class FitRayModel : public ofxNonLinearFit::Models::Base<glm::vec3, FitRayModel> {
			public:
				unsigned int getParameterCount() const override {
					return 5;
				}

				void getResidual(glm::vec3 dataPoint, double & residual, double * gradient = 0) const override {
					residual = this->ray.distanceTo(dataPoint);
				}

				void evaluate(glm::vec3 & dataPoint) const override {
					//we shouldn't be here
				}

				void cacheModel() override
				{
					this->ray.s.x = this->parameters[0];
					this->ray.s.y = this->parameters[1];
					this->ray.s.z = this->parameters[2];

					auto theta = this->parameters[3];
					auto thi = this->parameters[4];
					this->ray.t.x = cos(theta) * cos(thi);
					this->ray.t.y = sin(theta) * cos(thi);
					this->ray.t.z = sin(thi);
				}
				
				ofxRay::Ray ray;
			};

			struct FitPositionOnRayDataPoint {
				float dummy;
			};

			class FitPositionOnRayModel : public ofxNonLinearFit::Models::Base<FitPositionOnRayDataPoint, FitPositionOnRayModel> {
			public:
				unsigned int getParameterCount() const override {
					return 3; // for some reason, 1 parameter doesn't work
				}

				void getResidual(FitPositionOnRayDataPoint dataPoint, double & residual, double * gradient = 0) const override {
					auto projected = Utils::applyTransform(this->projectorViewProjection, this->world);
					residual = glm::distance(glm::vec2(projected.x, projected.y), targetPosition);
				}

				virtual void evaluate(FitPositionOnRayDataPoint &) const override {
					RULR_ERROR << "We shouldn't be here";
				}

				void cacheModel() override {
					world = this->ray.s + glm::normalize(this->ray.t) * this->parameters[0];
				}

				glm::vec2 targetPosition;
				ofMatrix4x4 projectorViewProjection;
				ofxRay::Ray ray;

				glm::vec3 world;
			};

			//----------
			void pickLineEnds(Projector::Line & line) {
				float minDistanceToStart = numeric_limits<float>::max();
				float minDistanceToEnd = numeric_limits<float>::max();

				for (const auto & vertex : line.vertices) {
					auto projectorNormalizedXY = vertex.projectorNormalizedXY;
					//projectorNormalizedXY.y *= -1;
					auto distanceToStart = glm::distance2(projectorNormalizedXY, line.startProjector);
					auto distanceToEnd = glm::distance2(projectorNormalizedXY, line.endProjector);

					if (distanceToStart < minDistanceToStart) {
						minDistanceToStart = distanceToStart;
						line.startWorld = vertex.world;
					}

					if (distanceToEnd < minDistanceToEnd) {
						minDistanceToEnd = distanceToEnd;
						line.endWorld = vertex.world;
					}
				}
			}

			void FitLines::fit() {
				Utils::ScopedProcess scopedProcess("Fitting lines");

				this->throwIfMissingAnyConnection();
				auto world = this->getInput<World>();
				auto projectors = world->getProjectors();
				auto scopedProcessScanProjectors = Utils::ScopedProcess("Fit lines in projectors", false, projectors.size());
				for (auto projector : projectors) {
					if (!projector) {
						scopedProcessScanProjectors.nextChildProcess();
						continue;
					}

					auto scopedProcessScanProjector = Utils::ScopedProcess("Fit lines for " + projector->getName(), false, 100);
					{
						auto itemProjector = projector->getInput<Item::Projector>();
						auto projectorView = itemProjector->getViewInWorldSpace();
						auto projectorViewProjection = projectorView.getViewMatrix() * projectorView.getClippedProjectionMatrix();

						auto & lines = projector->getLines();
						auto countPerPercent = lines.size() / 100;
						size_t count = 0;
						for (auto & line : lines) {
							if (count++ % countPerPercent == 0) {
								Utils::ScopedProcess scopedProcessPercent("Fitting line " + ofToString(count) + ", index " + ofToString(line.lineIndex) + " by " + line.lastEditBy);
							}

							if (line.vertices.size() < 2) {
								continue;
							}
							vector<glm::vec3> dataPoints;

							glm::vec3 mean;
							{
								for (const auto & vertex : line.vertices) {
									mean += vertex.world;
									dataPoints.emplace_back(vertex.world);
								}
								mean /= line.vertices.size();
							}

							ofxRay::Ray ray;
							double residual;

							//first fit
							{
								ofxNonLinearFit::Fit<FitRayModel> fit;
								FitRayModel model;
								model.ray.s = mean;

								//get the (infinite) ray which fits the vertices
								fit.optimise(model, &dataPoints, &residual);
								cout << residual << endl;
								line.startWorld = model.ray.getStart();
								line.endWorld = model.ray.getEnd();
								ray = model.ray;
							}


							

							

							//the above is working!

							if (residual < this->parameters.pick.maxResidualForFit) {
								//fit start and end
								{
									FitPositionOnRayModel model;
									model.projectorViewProjection = projectorViewProjection;
									model.ray = ray;

									//fit start 
									{
										model.targetPosition = line.startProjector;
										ofxNonLinearFit::Fit<FitPositionOnRayModel> fit;
										vector<FitPositionOnRayDataPoint> dataSet(1);
										double residual;
										fit.optimise(model, &dataSet, &residual);
										cout << "r2 : " << residual << endl;
										line.startWorld = model.world;
									}

									//fit end
									//fit start 
									{
										model.targetPosition = line.endProjector;
										ofxNonLinearFit::Fit<FitPositionOnRayModel> fit;
										vector<FitPositionOnRayDataPoint> dataSet(1);
										double residual;
										fit.optimise(model, &dataSet, &residual);
										cout << "r2 : " << residual << endl;
										line.endWorld = model.world;
									}
								}
							}
							else {
								pickLineEnds(line);
							}
						}
					}
				}

				scopedProcess.end();
			}

			//----------
			void FitLines::pick() {
				Utils::ScopedProcess scopedProcess("Picking lines");

				this->throwIfMissingAnyConnection();
				auto world = this->getInput<World>();
				auto projectors = world->getProjectors();
				const auto distanceThreshold = this->parameters.pick.distance.get();
				const auto distanceThreshold2 = distanceThreshold * distanceThreshold;

				auto scopedProcessScanProjectors = Utils::ScopedProcess("Pick line end points from projector vertices", false, projectors.size());
				for (auto projector : projectors) {
					if (!projector) {
						scopedProcessScanProjectors.nextChildProcess();
						continue;
					}

					auto scopedProcessScanProjector = Utils::ScopedProcess("Fit lines for " + projector->getName(), false);
					{
						auto & lines = projector->getLines();
						for (auto & line : lines) {
							if (line.vertices.size() < 2) {
								continue;
							}
							pickLineEnds(line);
						}
					}
					break;
				}
				scopedProcess.end();
			}

			//----------
			void FitLines::fullAuto() {
				this->throwIfMissingAConnection<World>();
				auto world = this->getInput<World>();
				auto projectors = world->getProjectors();

				Utils::ScopedProcess scopedProcess("Full auto", true, projectors.size());

				auto result = ofSystemLoadDialog("Load lines.json from mapping");
				if (!result.bSuccess) {
					return;
				}

				for (auto projector : projectors) {
					Utils::ScopedProcess scopedProcessProjector(projector->getName());
					projector->triangulate(0.05f);
					projector->loadMapping(result.filePath);
					projector->calibrateProjector();
					projector->dipLinesInData();
				}

				this->fit();
				scopedProcess.end();
			}

			//----------
			void FitLines::saveResults() {
				this->throwIfMissingAConnection<World>();

				auto world = this->getInput<World>();
				auto projectors = world->getProjectors();
				Utils::ScopedProcess scopedProcess("Saving", true, projectors.size());

				auto result = ofSystemSaveDialog("lines.json", "Load lines.json with 3D information");
				if (!result.bSuccess) {
					return;
				}

				json jsonLines;

				for (auto projector : projectors) {
					const auto & lines = projector->getLines();
					for (auto & line : lines) {
						json jsonLine;
						jsonLine["LineIndex"] = line.lineIndex;
						jsonLine["ProjectorIndex"] = line.projectorIndex;
						jsonLine["Start"]["x"] = line.startProjector.x;
						jsonLine["Start"]["y"] = line.startProjector.y;
						jsonLine["End"]["x"] = line.endProjector.x;
						jsonLine["End"]["y"] = line.endProjector.y;
						jsonLine["StartWorld"]["x"] = line.startWorld.x;
						jsonLine["StartWorld"]["y"] = line.startWorld.y;
						jsonLine["StartWorld"]["z"] = line.startWorld.z;
						jsonLine["EndWorld"]["x"] = line.endWorld.x;
						jsonLine["EndWorld"]["y"] = line.endWorld.y;
						jsonLine["EndWorld"]["z"] = line.endWorld.z;
						jsonLine["LastUpdate"] = line.lastUpdate;
						jsonLine["LastEditBy"] = line.lastEditBy;
						jsonLine["Age"] = line.age;
						jsonLines.push_back(jsonLine);
					}
				}

				ofFile file;
				file.open(result.filePath, ofFile::WriteOnly, false);
				file << jsonLines.dump();

				scopedProcess.end();
			}
		}
	}
}
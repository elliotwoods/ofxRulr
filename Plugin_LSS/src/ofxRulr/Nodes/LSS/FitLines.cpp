#include "pch_Plugin_LSS.h"

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
				}, OF_KEY_RETURN)->setHeight(100.0f);
				inspector->addButton("Pick lines", [this]() {
					try {
						this->pick();
					}
					RULR_CATCH_ALL_TO_ALERT;
				})->setHeight(100.0f);
			}

			//----------
			struct DataPoint {
				float x; // position along string
				ofVec3f position;
			};

			//----------
			class FitLineModel : public ofxNonLinearFit::Models::Base<DataPoint, FitLineModel> {
			public:
				virtual unsigned int getParameterCount() const override {
					return 6;
				}

				virtual void getResidual(DataPoint dataPoint, double & residual, double * gradient = 0) const override {
					auto actualPosition = dataPoint.position;
					this->evaluate(dataPoint);
					residual = (double) dataPoint.position.distance(actualPosition);
				}

				virtual void evaluate(DataPoint & dataPoint) const override {
					dataPoint.position = this->lineStart + (this->lineEnd - this->lineStart) * dataPoint.x;
				}

				virtual void cacheModel() override
				{
					this->lineStart[0] = this->parameters[0];
					this->lineStart[1] = this->parameters[1];
					this->lineStart[2] = this->parameters[2];
					this->lineEnd[0] = this->parameters[3];
					this->lineEnd[1] = this->parameters[4];
					this->lineEnd[2] = this->parameters[5];
				}
				
				ofVec3f lineStart;
				ofVec3f lineEnd;
			};

			//----------
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

					auto scopedProcessScanProjector = Utils::ScopedProcess("Fit lines for " + projector->getName(), false);
					{
						auto & lines = projector->getLines();
						for (auto & line : lines) {
							if (line.vertices.size() < 2) {
								continue;
							}
							vector<DataPoint> dataPoints;
							for (const auto & vertex : line.vertices) {
								auto x = (vertex.projectorNormalizedXY - line.startProjector).dot((line.endProjector - line.startProjector).getNormalized());
								x /= (line.endProjector - line.startProjector).length();

								dataPoints.emplace_back(DataPoint {
									x
									, vertex.world
								});
							}
							ofxNonLinearFit::Fit<FitLineModel> fit;
							FitLineModel model;
							double residual;
 							fit.optimise(model, &dataPoints, &residual);
 							cout << residual << endl;
 							line.startWorld = model.lineStart;
 							line.endWorld = model.lineEnd;
						}
					}
					break;
				}
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

							vector<DataPoint> dataPoints;
							size_t startCount = 0;
							size_t endCount = 0;
							ofVec3f start, end;
							for (const auto & vertex : line.vertices) {
								if (vertex.projectorNormalizedXY.squareDistance(line.startProjector) < distanceThreshold2) {
									start += vertex.world;
									startCount++;
								}
								if (vertex.projectorNormalizedXY.squareDistance(line.endProjector) < distanceThreshold2) {
									end += vertex.world;
									endCount++;
								}
							}

							start /= startCount;
							end /= endCount;

							cout << start << ", " << startCount << endl;
							cout << end << ", " << endCount << endl;
							cout << endl;

							if (startCount == 0 || endCount == 0) {
								continue;
							}

							line.startWorld = start;
							line.endWorld = end;
						}
					}
					break;
				}
				scopedProcess.end();
			}

		}
	}
}
#include "pch_RulrNodes.h"
#include "ProCamModel.h"

#include "ofxNonLinearFit.h"
#include "AddView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					//----------
					Model::Model(shared_ptr<AddView> camera, shared_ptr<AddView> projector) {
						{
							auto parameters = camera->getActiveFitParameters();
							this->fitParameters.insert(this->fitParameters.end(), parameters.begin(), parameters.end());
						}
						{
							auto parameters = projector->getActiveFitParameters();
							this->fitParameters.insert(this->fitParameters.end(), parameters.begin(), parameters.end());
						}

						this->camera = dynamic_pointer_cast<Item::Camera>(camera->getInput<Item::View>());
						this->projector = dynamic_pointer_cast<Item::Projector>(projector->getInput<Item::View>());

						this->multiThreaded = true;
					}

					//----------
					unsigned int Model::getParameterCount() const {
						return this->fitParameters.size();
					}

					//----------
					double Model::getResidual(AddScan::DataPoint dataPoint) const {
						const auto cameraRay = this->cameraCached.castCoordinate(dataPoint.camera);
						const auto projectorRay = this->projectorCached.castCoordinate(dataPoint.projector);
						const auto intersectRay = cameraRay.intersect(projectorRay);
						return intersectRay.getLengthSquared();
					}

					//----------
					void Model::evaluate(AddScan::DataPoint &) const {
						//do nothing
					}

					//----------
					void Model::cacheModel() {
						for (int i = 0; i < this->fitParameters.size(); i++) {
							this->fitParameters[i]->set(this->parameters[i]);
						}
						this->cameraCached = this->camera->getViewInWorldSpace();
						this->projectorCached = this->projector->getViewInWorldSpace();
					}

					//----------
					void Model::resetParameters() {
						for (int i = 0; i < this->fitParameters.size(); i++) {
							this->parameters[i] = this->fitParameters[i]->get();
						}
					}

					//----------
					vector<double> Model::getLowerBounds() const {
						vector<double> bounds(this->getParameterCount());
						for (int i = 0; i < this->fitParameters.size(); i++) {
							bounds[i] = this->fitParameters[i]->get() - this->fitParameters[i]->deviation / 2.0f;
						}
						return bounds;
					}

					//----------
					vector<double> Model::getUpperBounds() const {
						vector<double> bounds(this->getParameterCount());
						for (int i = 0; i < this->fitParameters.size(); i++) {
							bounds[i] = this->fitParameters[i]->get() + this->fitParameters[i]->deviation / 2.0f;
						}
						return bounds;
					}
				}
			}
		}
	}
}

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace ProCamSolve {
#pragma mark Model
				//----------
				SolveProjector::Model::Model() {
				}

				//----------
				unsigned int SolveProjector::Model::getParameterCount() const {
					return 10;
				}

				//----------
				void SolveProjector::Model::getResidual(DataPoint dataPoint, double & residual, double * gradient) const {
					if (gradient) {
						ofLogError("SolveProjector") << "This model does not support distortion";
					}
					auto cameraRay = this->cameraView.castPixel(dataPoint.cameraUndistorted);
					auto projectorRay = this->projectorView.castPixel(dataPoint.projector);
					auto intersection = cameraRay.intersect(projectorRay);
					residual = (double)intersection.getLengthSquared();
				}

				//----------
				void SolveProjector::Model::evaluate(DataPoint & dataPoint) const {
					// Shouldn't be here
				}

				//----------
				void SolveProjector::Model::cacheModel() {
					this->current.rotationVector = this->startingParameters.rotationVector.clone();
					this->current.translation = this->startingParameters.translation.clone();
					this->current.cameraMatrix = this->startingParameters.cameraMatrix.clone();

					this->current.rotationVector.at<double>(0) = this->parameters[0];
					this->current.rotationVector.at<double>(1) = this->parameters[1];
					this->current.rotationVector.at<double>(2) = this->parameters[2];
					this->current.translation.at<double>(0) = this->parameters[3];
					this->current.translation.at<double>(1) = this->parameters[4];
					this->current.translation.at<double>(2) = this->parameters[5];
					this->current.cameraMatrix.at<double>(0, 0) = this->parameters[6];
					this->current.cameraMatrix.at<double>(1, 1) = this->parameters[7];
					this->current.cameraMatrix.at<double>(0, 2) = this->parameters[8];
					this->current.cameraMatrix.at<double>(1, 2) = this->parameters[9];

					this->projector->setIntrinsics(this->current.cameraMatrix);
					this->projector->setExtrinsics(this->current.rotationVector, this->current.translation);

					this->projectorView = this->projector->getViewInWorldSpace();
				}

				//----------
				void SolveProjector::Model::resetParameters() {
					// store the defaults
					this->startingParameters.cameraMatrix = this->projector->getCameraMatrix();
					this->projector->getExtrinsics(this->startingParameters.rotationVector, this->startingParameters.translation);

					this->parameters[0] = this->startingParameters.rotationVector.at<double>(0);
					this->parameters[1] = this->startingParameters.rotationVector.at<double>(1);
					this->parameters[2] = this->startingParameters.rotationVector.at<double>(2);
					this->parameters[3] = this->startingParameters.translation.at<double>(0);
					this->parameters[4] = this->startingParameters.translation.at<double>(1);
					this->parameters[5] = this->startingParameters.translation.at<double>(2);
					this->parameters[6] = startingParameters.cameraMatrix.at<double>(0, 0);
					this->parameters[7] = startingParameters.cameraMatrix.at<double>(1, 1);
					this->parameters[8] = startingParameters.cameraMatrix.at<double>(0, 2);
					this->parameters[9] = startingParameters.cameraMatrix.at<double>(1, 2);

					this->cameraView = this->camera->getViewInWorldSpace();
					this->cameraView.distortion.clear(); // we work in undistorted coords
				}
#pragma mark SolveProjector
				//----------
				SolveProjector::SolveProjector() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string SolveProjector::getTypeName() const {
					return "Experiments::ProCamSolve::SolveProjector";
				}

				//----------
				void SolveProjector::init() {
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<Item::Projector>();
					this->addInput<Procedure::Scan::Graycode>();

					this->manageParameters(this->parameters);
				}

				//----------
				void SolveProjector::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;

					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
				}

				//----------
				void SolveProjector::calibrate() {
					Utils::ScopedProcess scopedProcess("Calibrate");

					auto camera = this->getInput<Item::Camera>();
					auto projector = this->getInput<Item::Projector>();

					auto initialDistance = glm::distance(camera->getPosition(), projector->getPosition());

					Model model;
					vector<DataPoint> dataPoints;
					{
						Utils::ScopedProcess scopedProcessPrepareData("Prepare data and model");

						this->throwIfMissingAnyConnection();
						auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();
						if (!graycodeNode->hasData()) {
							throw(ofxRulr::Exception("No Graycode dataset available"));
						}

						auto cameraMatrix = camera->getCameraMatrix();
						auto distortionCoefficients = camera->getDistortionCoefficients();


						// Build up dataSet
						const auto & dataSet = graycodeNode->getDataSet();
						int index = 0;
						for (const auto & pixel : dataSet) {
							// decimate the dataset
							if (index++ % this->parameters.dataSet.decimate.get() != 0) {
								continue;
							}

							// check graycode data pixel passes our threshold
							if (pixel.distance < this->parameters.dataSet.threshold) {
								continue;
							}

							auto cameraXY = pixel.getCameraXY();

							DataPoint dataPoint;
							dataPoint.cameraUndistorted = ofxCv::toOf(ofxCv::undistortPixelCoordinates(vector<cv::Point2f>(1, ofxCv::toCv(cameraXY))
								, cameraMatrix
								, distortionCoefficients).front());
							dataPoint.projector = pixel.getProjectorXY();

							dataPoints.emplace_back(move(dataPoint));
						}

						model.camera = camera;
						model.projector = projector;

						scopedProcessPrepareData.end();
					}

					// get the residual before we fit
					double startingResidual;
					{
						model.initialiseParameters();
						model.cacheModel();
						model.getResidualOnSet(dataPoints
							, startingResidual
							, NULL);
					}

					cout << "Starting residual = " << startingResidual << endl;

					double finalResidual;
					{
						Utils::ScopedProcess scopedProcessFit("Fit");

						ofxNonLinearFit::Fit<Model> fit;
						fit.optimise(model, &dataPoints, &finalResidual);

						scopedProcessFit.end();
					}

					cout << "Fit residual = " << finalResidual << endl;

					// test final fit
					{
						bool failed = false;

						failed != !isfinite(finalResidual);
						failed != finalResidual > startingResidual;

						if (failed) {
							model.resetParameters(); // use starting parameters
							model.cacheModel(); // push parameters over to projector
							throw(ofxRulr::Exception("Failed to find a better set of parameters with fit"));
						}
					}

					if (this->parameters.treatment.retainDistance) {
						auto cameraPosition = camera->getPosition();
						auto cameraToProjector = projector->getPosition() - cameraPosition;
						auto newDistance = glm::length(cameraToProjector);
						auto treatedCameraToProjector = initialDistance / newDistance * cameraToProjector;
						projector->setPosition(cameraPosition + treatedCameraToProjector);
					}

					scopedProcess.end();
				}
			}
		}
	}
}
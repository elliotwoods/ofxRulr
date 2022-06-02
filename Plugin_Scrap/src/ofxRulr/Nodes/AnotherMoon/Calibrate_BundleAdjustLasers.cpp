#include "pch_Plugin_Scrap.h"
#include "Calibrate.h"
#include "Lasers.h"
#include "ofxRulr/Solvers/BundleAdjustmentLasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			void
				Calibrate::calibrateBundleAdjustLasers(bool performSolve)
			{
				Utils::ScopedProcess scopedProcess(performSolve ? "Bundle adjust points" : "Calculate residuals");

				this->throwIfMissingAConnection<Lasers>();
				this->throwIfMissingAConnection<Item::Camera>();

				auto cameraNode = this->getInput<Item::Camera>();
				auto lasersNode = this->getInput<Lasers>();

				auto cameraCaptures = this->cameraCaptures.getSelection();

				// First deselect lasers seen in fewer than 2 cameras
				this->deselectLasersWithNoData(2);

				auto selectedLasers = lasersNode->getLasersSelected();

				// Build initial solution
				Solvers::BundleAdjustmentLasers::Solution initialSolution;
				{
					for (auto laser : selectedLasers) {
						initialSolution.laserProjectors.push_back(laser->getModel());
					}

					for (auto cameraCapture : cameraCaptures) {
						auto rigidBodyTransform = cameraCapture->cameraTransform->getTransform();
						auto viewTransform = glm::inverse(rigidBodyTransform);
						initialSolution.cameraViewTransforms.emplace_back(viewTransform);
					}
				}

				// Build intrinsics
				Models::Intrinsics cameraIntrinsics;
				{
					cameraIntrinsics.width = cameraNode->getWidth();
					cameraIntrinsics.height = cameraNode->getHeight();
					cameraIntrinsics.projectionMatrix = cameraNode->getViewInObjectSpace().getProjectionMatrix();
				}

				// Initialise problem
				Solvers::BundleAdjustmentLasers::Problem problem(initialSolution
					, cameraIntrinsics);

				// Map laser addresses to indices
				map<size_t, int> laserAddressByIndex;
				map<int, size_t> laserIndexByAddress;
				{
					size_t index = 0;
					for (auto laser : selectedLasers) {
						auto address = laser->parameters.communications.address.get();

						if (laserIndexByAddress.find(address) != laserIndexByAddress.end()) {
							throw(ofxRulr::Exception("Duplicate laser address : " + ofToString(address)));
						}
						laserAddressByIndex[index] = address;
						laserIndexByAddress[address] = index;
						index++;
					}
				}

				// Gather images
				struct AssociatedImage {
					Solvers::BundleAdjustmentLasers::Image image;
					shared_ptr<BeamCapture> beamCapture;
				};
				vector<AssociatedImage> associatedImages;
				{
					for (size_t cameraIndex = 0; cameraIndex < cameraCaptures.size(); cameraIndex++) {
						auto cameraCapture = cameraCaptures[cameraIndex];
						auto laserCaptures = cameraCapture->laserCaptures.getSelection();
						for (auto laserCapture : laserCaptures) {

							// check laser is part of set
							auto findIndex = laserIndexByAddress.find(laserCapture->laserAddress);
							if (findIndex == laserIndexByAddress.end()) {
								continue;
							}

							auto beamCaptures = laserCapture->beamCaptures.getSelection();
							for (auto beamCapture : beamCaptures) {
								Solvers::BundleAdjustmentLasers::Image image;
								image.laserProjectorIndex = findIndex->second;
								image.cameraIndex = cameraIndex;
								image.projectedPoint = beamCapture->projectionPoint;
								image.imageLine = beamCapture->line;

								// Store alongside beamCapture (this is used to easily fill residuals later)
								associatedImages.push_back(AssociatedImage{
									image
									, beamCapture
									});
							}
						}
					}
				}

				// Add images to problem
				for (const auto& associatedImage : associatedImages) {
					problem.addLineImageObservation(associatedImage.image);
				}

				// Add scene constraints
				{
					// Scene center
					glm::vec3 sceneCenter;
					{
						glm::vec3 accumulator;
						for (auto laser : selectedLasers) {
							accumulator += laser->getRigidBody()->getPosition();
						}
						sceneCenter = accumulator / selectedLasers.size();

						if (this->parameters.bundleAdjustment.sceneCenterConstraint.get()) {
							problem.addLaserLayoutCenteredConstraint(sceneCenter);
						}
					}

					// Scene radius
					float sceneradius = 0.0f;
					{
						for (auto laser : selectedLasers) {
							auto distance = glm::distance(sceneCenter, laser->getRigidBody()->getPosition());
							sceneradius += distance;
						}
						sceneradius /= (float)selectedLasers.size();

						if (this->parameters.bundleAdjustment.sceneRadiusConstraint.get()) {
							problem.addLaserLayoutScaleConstraint(sceneradius);
						}
					}

					// Camera yaw fixed for first camera
					if (this->parameters.bundleAdjustment.cameraWith0Yaw.enabled) {
						problem.addCameraZeroYawConstraint(this->parameters.bundleAdjustment.cameraWith0Yaw.cameraIndex);
					}

					// Plane constraint
					if (this->parameters.bundleAdjustment.planeConstraint) {
						problem.addLasersInPlaneConstraint(1);
					}
				}

				// Options to set parameters as fixed
				{
					if (this->parameters.bundleAdjustment.fixed.lasers.position) {
						problem.setLaserPositionsFixed();
					}

					if (this->parameters.bundleAdjustment.fixed.lasers.rotation) {
						problem.setLaserRotationsFixed();
					}

					if (this->parameters.bundleAdjustment.fixed.lasers.fov) {
						problem.setLaserFOVsFixed();
					}

					if (this->parameters.bundleAdjustment.fixed.cameras) {
						problem.setCamerasFixed();
					}
				}
				

				// Solve the problem
				Solvers::BundleAdjustmentLasers::Solution solution;
				if (performSolve) {
					auto solverSettings = Solvers::BundleAdjustmentLasers::defaultSolverSettings();
					this->configureSolverSettings(solverSettings, this->parameters.bundleAdjustment.solverSettings);
					auto result = problem.solve(solverSettings);
					solution = result.solution;
				}
				else {
					solution = initialSolution;
				}

				// Pull the solution back out
				{
					for (const auto& it : laserAddressByIndex) {
						selectedLasers[it.first]->getRigidBody()->setTransform(solution.laserProjectors[it.first].rigidBodyTransform.getTransform());
						selectedLasers[it.first]->parameters.intrinsics.fov.set(solution.laserProjectors[it.first].fov);
					}

					for (size_t cameraIndex = 0; cameraIndex < cameraCaptures.size(); cameraIndex++) {
						auto cameraCapture = cameraCaptures[cameraIndex];
						const auto viewTransform = solution.cameraViewTransforms[cameraIndex].getTransform();
						const auto rigidBodyTransform = glm::inverse(viewTransform);
						cameraCapture->cameraTransform->setTransform(rigidBodyTransform);
					}
				}

				// Calculate and store residuals on images
				for (const auto& associatedImage : associatedImages) {
					auto residual = Solvers::BundleAdjustmentLasers::getResidual(solution
						, cameraIntrinsics
						, associatedImage.image);
					if (isnan(residual)) {
						// translate to laserAddress
						auto imageCopy = associatedImage.image;
						imageCopy.laserProjectorIndex = laserAddressByIndex[imageCopy.laserProjectorIndex];
						throw(ofxRulr::Exception("NaN residual for image : " + imageCopy.toString()));
					}
					associatedImage.beamCapture->residual = residual;
				}

				scopedProcess.end();
			}
		}
	}
}
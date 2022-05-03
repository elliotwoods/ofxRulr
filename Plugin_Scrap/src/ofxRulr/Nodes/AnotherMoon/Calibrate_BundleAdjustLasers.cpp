#include "pch_Plugin_Scrap.h"
#include "Calibrate.h"
#include "Lasers.h"
#include "ofxRulr/Solvers/BundleAdjustmentLasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			void
				Calibrate::calibrateBundleAdjustLasers()
			{
				Utils::ScopedProcess scopedProcess("Bundle adjust points");

				this->throwIfMissingAConnection<Lasers>();
				this->throwIfMissingAConnection<Item::Camera>();

				auto cameraNode = this->getInput<Item::Camera>();
				auto lasersNode = this->getInput<Lasers>();

				auto cameraCaptures = this->cameraCaptures.getSelection();

				// First deselect lasers seen in fewer than 2 cameras
				this->deselectLasersWithNoData(2);

				auto selectedLasers = lasersNode->getSelectedLasers();

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
						auto address = laser->parameters.settings.address.get();
						laserAddressByIndex[index] = address;
						laserIndexByAddress[address] = index;
						index++;
					}
				}

				// Add images to problem
				{
					for (size_t cameraIndex = 0; cameraIndex < cameraCaptures.size(); cameraIndex++) {
						auto cameraCapture = cameraCaptures[cameraIndex];
						auto laserCaptures = cameraCapture->laserCaptures.getSelection();
						for (auto laserCapture : laserCaptures) {
							auto beamCaptures = laserCapture->beamCaptures.getSelection();
							for (auto beamCapture : beamCaptures) {
								Solvers::BundleAdjustmentLasers::Image image;
								image.laserProjectorIndex = laserIndexByAddress[laserCapture->laserAddress];
								image.cameraIndex= cameraIndex;
								image.projectedPoint = beamCapture->projectionPoint;
								image.imageLine= beamCapture->line;
								problem.addLineImageObservation(image);
							}
						}
					}
				}

				//// Add scene constraints
				//{
				//	// Scene center
				//	glm::vec3 sceneCenter;
				//	{
				//		glm::vec3 accumulator;
				//		for (auto laser : selectedLasers) {
				//			accumulator += laser->getRigidBody()->getPosition();
				//		}
				//		sceneCenter = accumulator / selectedLasers.size();

				//		if (this->parameters.bundleAdjustment.sceneCenterConstraint.get()) {
				//			problem.addSceneCenteredConstraint(sceneCenter);
				//		}
				//	}

				//	// Scene radius
				//	float sceneRadius = 0.0f;
				//	{
				//		for (auto laser : selectedLasers) {
				//			auto distance = glm::distance(sceneCenter, laser->getRigidBody()->getPosition());
				//			if (distance > sceneRadius) {
				//				sceneRadius = distance;
				//			}
				//		}

				//		if (this->parameters.bundleAdjustment.sceneRadiusConstraint.get()) {
				//			problem.addSceneScaleConstraint(sceneRadius);
				//		}
				//	}

				//	// Camera yaw fixed for first camera
				//	if (this->parameters.bundleAdjustment.cameraWith0Yaw.enabled) {
				//		problem.addCameraZeroYawConstraint(this->parameters.bundleAdjustment.cameraWith0Yaw.cameraIndex);
				//	}

				//	// Plane constraint
				//	if (this->parameters.bundleAdjustment.planeConstraint) {
				//		problem.addPointsInPlaneConstraint(1);
				//	}
				//}

				// Solve the problem
				auto solverSettings = Solvers::BundleAdjustmentLasers::defaultSolverSettings();
				this->configureSolverSettings(solverSettings);
				auto result = problem.solve(solverSettings);
				auto& solution = result.solution;

				// Pull the solution back out
				{
					for (const auto& it : laserAddressByIndex) {
						selectedLasers[it.first]->getRigidBody()->setTransform(solution.laserProjectors[it.first].rigidBodyTransform.getTransform());
						selectedLasers[it.first]->parameters.settings.fov.set(solution.laserProjectors[it.first].fov);
					}

					for (size_t cameraIndex = 0; cameraIndex < cameraCaptures.size(); cameraIndex++) {
						auto cameraCapture = cameraCaptures[cameraIndex];
						const auto viewTransform = solution.cameraViewTransforms[cameraIndex].getTransform();
						const auto rigidBodyTransform = glm::inverse(viewTransform);
						cameraCapture->cameraTransform->setTransform(rigidBodyTransform);
					}
				}

				scopedProcess.end();
			}
		}
	}
}
#include "pch_Plugin_Scrap.h"
#include "Calibrate.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			void
				Calibrate::calibrateInitialCameras()
			{
				Utils::ScopedProcess scopedProcess("Initial camera poses");

				this->throwIfMissingAConnection<Lasers>();
				this->throwIfMissingAConnection<Item::Camera>();

				auto cameraNode = this->getInput<Item::Camera>();
				auto lasersNode = this->getInput<Lasers>();
				auto selectedLasers = lasersNode->getLasersSelected();

				auto cameraCaptures = this->cameraCaptures.getSelection();

				{
					Utils::ScopedProcess scopedProcessCameras("Cameras", false, cameraCaptures.size());

					for (auto cameraCapture : cameraCaptures) {
						Utils::ScopedProcess scopedProcessCameraCapture(cameraCapture->getName());

						// Gather data
						vector<glm::vec2> imagePoints;
						vector<glm::vec3> worldPoints;
						{
							auto laserCaptures = cameraCapture->laserCaptures.getSelection();
							for (auto laserCapture : laserCaptures) {
								// Get the laser that matches this capture
								auto laser = lasersNode->findLaser(laserCapture->laserAddress);
								if (!laser) {
									continue;
								}

								imagePoints.push_back(laserCapture->imagePointInCamera);
								worldPoints.push_back(laser->getRigidBody()->getPosition());
							}
						}

						// Perform the solve and store
						{
							auto useExtrinsicGuess = this->parameters.initialCameras.useExtrinsicGuess.get();

							cv::Mat rotationVector, translation;
							
							if (useExtrinsicGuess) {
								cameraCapture->cameraTransform->getExtrinsics(rotationVector
									, translation
									, true);
							}

							int flags = 0;
							{
								if (worldPoints.size() < 3) {
									throw(Exception("Cannot perform SolvePnP with fewer than 3 points"));
								}
								if (worldPoints.size() == 3) {
									flags |= cv::SolvePnPMethod::SOLVEPNP_AP3P;
								}
								else if (worldPoints.size() < 6) {
									flags |= cv::SolvePnPMethod::SOLVEPNP_EPNP;
								}
							}


							if (!cv::solvePnP(ofxCv::toCv(worldPoints)
								, ofxCv::toCv(imagePoints)
								, cameraNode->getCameraMatrix()
								, cameraNode->getDistortionCoefficients()
								, rotationVector
								, translation
								, useExtrinsicGuess
								, flags)) {
								throw(Exception("SolvePnP failed"));
							}

							cameraCapture->cameraTransform->setExtrinsics(rotationVector
								, translation
								, true);

							{
								auto reprojectionError = ofxCv::reprojectionError(ofxCv::toCv(imagePoints)
									, ofxCv::toCv(worldPoints)
									, rotationVector
									, translation
									, cameraNode->getCameraMatrix()
									, cameraNode->getDistortionCoefficients());
								cout << "Reprojection error " << reprojectionError << endl;
							}

							scopedProcessCameraCapture.end();
						}
					}
				}

				scopedProcess.end();
			}
		}
	}
}
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
							cv::Mat rotationVector, translation;
							
							cv::solvePnP(ofxCv::toCv(worldPoints)
								, ofxCv::toCv(imagePoints)
								, cameraNode->getCameraMatrix()
								, cameraNode->getDistortionCoefficients()
								, rotationVector
								, translation
								, false);

							cameraCapture->cameraTransform->setExtrinsics(rotationVector
								, translation
								, true);

							{
								cv::Mat translationI, rotationVectorI;
								cameraCapture->cameraTransform->getExtrinsics(translationI
									, rotationVectorI
									, true);
								auto reprojectionError = ofxCv::reprojectionError(ofxCv::toCv(imagePoints)
									, ofxCv::toCv(worldPoints)
									, rotationVectorI
									, translationI
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
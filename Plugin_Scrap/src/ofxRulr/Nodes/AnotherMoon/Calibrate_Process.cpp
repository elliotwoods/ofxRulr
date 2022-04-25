#include "pch_Plugin_Scrap.h"
#include "Calibrate.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			void
				Calibrate::process()
			{
				Utils::ScopedProcess scopedProcess("Process");

				this->throwIfMissingAConnection<Lasers>();
				this->throwIfMissingAConnection<Item::Camera>();

				auto cameraNode = this->getInput<Item::Camera>();
				auto lasersNode = this->getInput<Lasers>();
				auto selectedLasers = lasersNode->getSelectedLasers();

				auto cameraCaptures = this->cameraCaptures.getSelection();

				{
					Utils::ScopedProcess scopedProcessCameras("Cameras", false, cameraCaptures.size());

					for (auto cameraCapture : cameraCaptures) {
						auto laserCaptures = cameraCapture->laserCaptures.getSelection();
						Utils::ScopedProcess scopedProcessCameras("Camera : " + cameraCapture->getDateString() + " " + cameraCapture->getTimeString(), false, laserCaptures.size());

						for (auto laserCapture : laserCaptures) {
							auto beamCaptures = laserCapture->beamCaptures.getSelection();
							Utils::ScopedProcess scopedProcessLasers("Laser #" + ofToString(laserCapture->laserAddress), false, beamCaptures.size());

							vector<Solvers::LinesWithCommonPoint::CameraImagePoints> images;

							cv::Mat preview;

							// Gather image points per beam in laser
							for (auto beamCapture : beamCaptures) {
								Utils::ScopedProcess scopedProcessBeam("Beam : " + ofToString(beamCapture->imagePoint), false, beamCaptures.size());

								// Get the on and off images
								auto onImage = this->fetchImage(beamCapture->onImage);
								auto offImage = this->fetchImage(beamCapture->offImage);

								// Get the positive difference
								cv::Mat difference;
								cv::subtract(onImage, offImage, difference);

								// Allocate preview if needs be (unallocated = don't make preview)
								if (preview.empty()) {
									preview = cv::Mat::zeros(difference.size(), difference.type());
								}

								// Get the camera image points for this image
								images.push_back(Solvers::LinesWithCommonPoint::getCameraImagePoints(difference
									, cameraNode
									, this->parameters.processing.normalizePercentile.get()
									, this->parameters.processing.differenceThreshold.get()
									, preview));
							}

							// Solve lines for the whole laser using a common convergence point
							auto solverSettings = Solvers::LinesWithCommonPoint::defaultSolverSettings();
							this->configureSolverSettings(solverSettings);
							auto result = Solvers::LinesWithCommonPoint::solve(images
								, this->parameters.processing.distanceThreshold.get()
								, this->parameters.processing.minMeanPixelValueOnLine.get()
								, solverSettings);

							// Pull data back from solve
							laserCapture->imagePointInCamera = result.solution.point;
							for (int i = 0; i < beamCaptures.size(); i++) {
								beamCaptures[i]->line = result.solution.lines[i];
								if (!result.solution.linesValid[i]) {
									beamCaptures[i]->setSelected(false);
								}
								else {
									beamCaptures[i]->line.drawOnImage(preview);
								}
							}

							laserCapture->preview = preview;
						}
					}
				}

				scopedProcess.end();
			}
		}
	}
}
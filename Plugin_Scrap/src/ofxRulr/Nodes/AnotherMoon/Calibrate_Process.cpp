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
							try {
								// in case of exception this will persist
								{
									laserCapture->setSelected(false);
									laserCapture->linesWithCommonPointSolveResult.success = false;
									laserCapture->linesWithCommonPointSolveResult.residual = 0.0f;
								}

								// check if it's disabled
								{
									if (laserCapture->parameters.markBad) {
										throw(ofxRulr::Exception("This capture is marked as bad"));
									}
								}

								const size_t minBeamCapture = 4;

								auto beamCaptures = laserCapture->beamCaptures.getSelection();
								Utils::ScopedProcess scopedProcessLasers("Laser #" + ofToString(laserCapture->laserAddress), false);

								// Gather solveData and parameters
								if (beamCaptures.size() < minBeamCapture) {
									throw(ofxRulr::Exception("Too few beam captures (" + ofToString(beamCaptures.size()) + ") for laser #" + ofToString(laserCapture->laserAddress)));
								}

								// Gather solveData and parameters
								Solvers::LinesWithCommonPoint::SolveData solveData;
								{
									solveData.cameraNode = cameraNode;
									solveData.normalizePercentile = this->parameters.processing.normalizePercentile.get();
									solveData.differenceThreshold = this->parameters.processing.differenceThreshold.get();
									solveData.distanceThreshold = this->parameters.processing.distanceThreshold.get();
									solveData.minMeanPixelValueOnLine = this->parameters.processing.minMeanPixelValueOnLine.get();

									solveData.debug.previewEnabled = this->parameters.processing.preview.enabled.get();
									solveData.debug.previewPopup = this->parameters.processing.preview.popup.get();
									solveData.debug.previewSize = cv::Size(cameraNode->getWidth(), cameraNode->getHeight());
									solveData.debug.directory = laserCapture->directory;
								}

								// Gather beams (load the files)
								{
									Utils::ScopedProcess scopedProcessGatherBeams("Gather beams", true, beamCaptures.size());
									int beamIndex = 0;
									for (auto beamCapture : beamCaptures) {
										Utils::ScopedProcess scopedProcessBeam("Beam #" + ofToString(beamCapture->imagePoint));

										solveData.onImages.push_back(this->fetchImage(beamCapture->onImage));
										solveData.offImages.push_back(this->fetchImage(beamCapture->offImage));
										beamIndex++;

										scopedProcessBeam.end();
									}
									scopedProcessGatherBeams.end();
								}

								// Solve the lines with common convergence point
								{
									Utils::ScopedProcess scopedProcessSolve("Solve lines and convergence");

									// Perform solve
									auto solverSettings = Solvers::LinesWithCommonPoint::defaultSolverSettings();
									this->configureSolverSettings(solverSettings);
									auto result = Solvers::LinesWithCommonPoint::solve(solveData, solverSettings);

									// Save the preview as report
									if (this->parameters.processing.preview.enabled && this->parameters.processing.preview.save) {
										cv::imwrite((laserCapture->directory / "Report - LinesWithCommonPoint.png").string(), result.solution.preview);
									}

									// Pull data back from solve
									for (int i = 0; i < beamCaptures.size(); i++) {
										beamCaptures[i]->line = result.solution.lines[i];
										if (!result.solution.linesValid[i]) {
											cout << "Beam " << i << " is not valid and will be deselected" << endl;
											beamCaptures[i]->setSelected(false);
										}
									}

									// Check we still have enough data
									{
										auto validBeamCaptures = laserCapture->beamCaptures.getSelection();
										if (validBeamCaptures.size() < minBeamCapture) {
											throw(ofxRulr::Exception("Too few beams pass the line finding test"));
										}
									}

									// Save the result if valid
									{
										laserCapture->linesWithCommonPointSolveResult.success = result.isConverged();
										laserCapture->linesWithCommonPointSolveResult.residual = result.residual;

										laserCapture->imagePointInCamera = result.solution.point;
										laserCapture->preview = result.solution.preview;
									}

									scopedProcessSolve.end();
								}

								scopedProcessLasers.end();
								laserCapture->setSelected(true);
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				}

				scopedProcess.end();
			}
		}
	}
}
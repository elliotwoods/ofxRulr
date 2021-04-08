#include "pch_Plugin_Scrap.h"
#include "FindLine.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/CircleLaser.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//---------
				FindLine::FindLine() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string FindLine::getTypeName() const {
					return "Procedure::Calibrate::FindLine";
				}

				//---------
				void FindLine::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<Item::CircleLaser>();

					auto groupPanel = ofxCvGui::Panels::Groups::makeStrip();

					{
						auto panel = ofxCvGui::Panels::makeImage(this->previewDifference);

						panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
							if (this->previewFrame) {
								//draw first guess
								{
									auto lineStart = glm::vec2(this->previewFrame->lineFirstTry[2], this->previewFrame->lineFirstTry[3]);
									auto lineTransmit = glm::vec2(this->previewFrame->lineFirstTry[0], this->previewFrame->lineFirstTry[1]);

									ofPushStyle();
									{
										//unfiltered points
										ofSetColor(100, 100, 0);
										this->previewPoints.draw();

										//forwards line
										ofSetColor(100, 0, 0);
										ofDrawLine(lineStart, lineStart + lineTransmit * 1000.0f);

										//backwards line
										ofSetColor(0, 0, 100);
										ofDrawLine(lineStart, lineStart - lineTransmit * 1000.0f);
									}
									ofPopStyle();
								}

								//draw final line
								{
									auto lineStart = glm::vec2(this->previewFrame->line[2], this->previewFrame->line[3]);
									auto lineTransmit = glm::vec2(this->previewFrame->line[0], this->previewFrame->line[1]);

									ofPushStyle();
									{
										//points
										ofSetColor(0, 255, 0);
										this->previewFilteredPoints.draw();

										//forwards line
										ofSetColor(255, 0, 0);
										ofDrawLine(lineStart, lineStart + lineTransmit * 1000.0f);

										//backwards line
										ofSetColor(0, 0, 255);
										ofDrawLine(lineStart, lineStart - lineTransmit * 1000.0f);
									}
									ofPopStyle();
								}
							}
						};
						groupPanel->add(panel);
					}

					{
						auto panel = ofxCvGui::Panels::makeImage(this->previewBinary);
						groupPanel->add(panel);
					}

					{
						auto panel = ofxCvGui::Panels::makeImage(this->previewMaskedBinary);
						groupPanel->add(panel);
					}
					

					this->panel = groupPanel;
				}

				//---------
				void FindLine::update() {
					auto cachedPreviewFrame = this->cachedPreviewFrame.lock();
					if (cachedPreviewFrame != this->previewFrame) {
						//update the preview
						if (!this->previewFrame) {
							this->previewDifference.clear();
							this->previewBinary.clear();
							this->previewMaskedBinary.clear();
						}
						else {
							ofxCv::copy(this->previewFrame->difference, this->previewDifference.getPixels());
							ofxCv::copy(this->previewFrame->binary, this->previewBinary.getPixels());
							ofxCv::copy(this->previewFrame->maskedDifference, this->previewMaskedBinary.getPixels());

							this->previewDifference.update();
							this->previewBinary.update();
							this->previewMaskedBinary.update();
						}

						this->previewPoints.clear();
						for (const auto & vertex : this->previewFrame->points) {
							this->previewPoints.addVertex({
								vertex.x
								, vertex.y
								, 0.0f });
						}
						this->previewPoints.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

						this->previewFilteredPoints.clear();
						for (const auto & vertex : this->previewFrame->filteredPoints) {
							this->previewFilteredPoints.addVertex({
								vertex.x
								, vertex.y
								, 0.0f}); 
						}
						this->previewFilteredPoints.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

						this->cachedPreviewFrame = this->previewFrame;
					}
				}

				//---------
				ofxCvGui::PanelPtr FindLine::getPanel() {
					return this->panel;
				}

				//---------
				void FindLine::serialize(nlohmann::json & json) {
					Utils::serialize(json, this->parameters);
				}

				//---------
				void FindLine::deserialize(const nlohmann::json & json) {
					Utils::deserialize(json, this->parameters);
				}

				//---------
				void FindLine::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addParameterGroup(this->parameters);

					inspector->addButton("Find  line", [this]() {
						try {
							this->capture();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');

					inspector->addButton("Recalculate", [this]() {
						try {
							this->recalculate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, 'r');
				}

				//---------
				shared_ptr<FindLine::OutputFrame> FindLine::findLine(ofPixels foreground, ofPixels background) {
					Utils::ScopedProcess scopedProcessFindLine("Find line");

					auto outputFrame = make_shared<OutputFrame>();

					outputFrame->foregroundPixels = foreground;
					outputFrame->backgroundPixels = background;

					outputFrame->foreground = ofxCv::toCv(foreground);
					outputFrame->background = ofxCv::toCv(background);

					auto toGray = [](ofPixels & pixels, cv::Mat & image){
						//convert to grayscale if needs be
						switch (pixels.getPixelFormat()) {
						case ofPixelFormat::OF_PIXELS_GRAY:
							break;
						case ofPixelFormat::OF_PIXELS_RGB:
						case ofPixelFormat::OF_PIXELS_BGR:
							cv::cvtColor(image, image, cv::COLOR_RGB2GRAY);
							break;
						case ofPixelFormat::OF_PIXELS_RGBA:
						case ofPixelFormat::OF_PIXELS_BGRA:
							cv::cvtColor(image, image, cv::COLOR_RGBA2GRAY);
							break;
						default:
							throw(ofxRulr::Exception("Image format not supported by FindContourMarkers"));
						}
					};
					
					toGray(foreground, outputFrame->foreground);
					toGray(background, outputFrame->background);
					

					//perform local difference filter
					{
						//cv::blur(outputFrame->image, outputFrame->blur, cv::Size(this->parameters.localDifference.blurSizeX, this->parameters.localDifference.blurSizeY));
						outputFrame->difference = outputFrame->foreground - outputFrame->background;
						cv::imwrite("foreground.png", outputFrame->foreground);
						cv::imwrite("background.png", outputFrame->background);

						outputFrame->difference *= this->parameters.localDifference.differenceAmplify;

						cv::threshold(outputFrame->difference
							, outputFrame->binary
							, this->parameters.localDifference.threshold
							, 255
							, cv::THRESH_BINARY);

						auto element = cv::getStructuringElement(cv::MORPH_RECT,
							cv::Size(2 * this->parameters.localDifference.erosionSize + 1, 2 * this->parameters.localDifference.erosionSize + 1),
							cv::Point(this->parameters.localDifference.erosionSize, this->parameters.localDifference.erosionSize));

						if (this->parameters.localDifference.erosionSize > 0) {
							cv::erode(outputFrame->binary, outputFrame->binary, element);
						}

						//create a masked difference image
						{
							cv::Mat binaryDilated;
							cv::dilate(outputFrame->binary, binaryDilated, element, cv::Point(-1, -1), 2);
							cv::bitwise_and(outputFrame->difference, binaryDilated, outputFrame->maskedDifference);
						}
					}

					//build up a dataset for the line fit
					{
						const auto width = outputFrame->difference.cols;
						const auto height = outputFrame->difference.rows;
						const auto boxSize = this->parameters.lineFit.boxSize.get();

						auto testBox = [outputFrame, this](const cv::Rect & roi) {
							//for each little section of the image
							auto subImageBinary = outputFrame->binary(roi);

							/*
							//check there's something which passed threshold in this section
							if (cv::countNonZero(subImageBinary) >= this->parameters.lineFit.minimumCountInBox) {
								auto subImage = outputFrame->maskedDifference(roi);

								auto moments = cv::moments(subImage);

								outputFrame->points.emplace_back(
									moments.m10 / moments.m00 + roi.x
									, moments.m01 / moments.m00 + roi.y
								);
							}*/

							vector<cv::Point2i> nonZeroLocations;
							cv::findNonZero(subImageBinary, nonZeroLocations);
							for (const auto & location : nonZeroLocations) {
								outputFrame->points.push_back(location + cv::Point2i(roi.x, roi.y));
							}

						};

						//check all vertical slices
						if (this->parameters.lineFit.enableVerticalBoxes) {
							for (int x = 0; x + boxSize < width; x += boxSize) {
								auto roi = cv::Rect(x, 0, boxSize, height);
								testBox(roi);
							}
						}

						//check all horizontal slices
						for (int y = 0; y + boxSize < height; y += boxSize) {
							auto roi = cv::Rect(0, y, width, boxSize);
							testBox(roi);
						}
					}

// 					//fit hough line
// 					{
// 						vector<cv::Vec2f> lines;
// 						cv::HoughLines(outputFrame->binary, lines, 0.01, TWO_PI / 1000.0, 30);
// 						for (const auto & line : lines) {
// 							cout << line << endl;
// 						}
// 					}

					//fit the line
					{
						//first try (take all points)
						cv::fitLine(outputFrame->points
							, outputFrame->lineFirstTry
							, cv::DIST_FAIR
							, 0
							, 0.01
							, 0.01);

						//filter all the points out which don't match the line
						cv::Vec2f lineOrigin(outputFrame->lineFirstTry[2], outputFrame->lineFirstTry[3]);
						cv::Vec2f lineTransmission(outputFrame->lineFirstTry[0], outputFrame->lineFirstTry[1]);

						auto distanceThreshold2 = this->parameters.lineFit.distanceThreshold.get();
						distanceThreshold2 *= distanceThreshold2;

						for (const auto & point : outputFrame->points) {
							const auto & pointV = (cv::Vec2f &) point;

							//calculate distance point to line
							auto lineOriginToPoint = pointV - lineOrigin;

							//calculate projection of point onto line
							auto projectedPoint = lineOriginToPoint.dot(lineTransmission) * lineTransmission + lineOrigin;

							//calculate distance
							auto delta = pointV - projectedPoint;
							auto distance2 = delta[0] * delta[0] + delta[1] * delta[1];

							if (distance2 <= distanceThreshold2) {
								outputFrame->filteredPoints.emplace_back(point);
							}
						}

						//refined fit
						cv::fitLine(outputFrame->filteredPoints
							, outputFrame->line
							, cv::DIST_HUBER
							, 0
							, 0.001
							, 0.001);
					}

					scopedProcessFindLine.end();
					
					this->previewFrame = outputFrame;

					return outputFrame;
				}

				//---------
				void FindLine::recalculate() {
					if (!this->previewFrame) {
						throw(ofxRulr::Exception("No frame to recalculate"));
					}

					this->previewFrame = this->findLine(this->previewFrame->foregroundPixels
						, this->previewFrame->backgroundPixels);
				}

				//---------
				void FindLine::capture() {
					Utils::ScopedProcess scopedProcess("Capture line", true, 2);

					this->throwIfMissingAnyConnection();
					auto camera = this->getInput<Item::Camera>();
					auto grabber = camera->getGrabber();

					Utils::ScopedProcess scopedProcessForeground("Capture foreground", false);
					auto foregroundFrame = grabber->getFreshFrame(chrono::seconds(20));
					if (!foregroundFrame || !foregroundFrame->getPixels().isAllocated()) {
						throw(ofxRulr::Exception("Failed to capture incoming frame"));
					}
					const auto foreground = foregroundFrame->getPixels();
					scopedProcessForeground.end();


					Utils::ScopedProcess scopedProcessBackground("Capture background", false);
					//clear for background
					this->getInput<Item::CircleLaser>()->clearDrawing();

					auto backgroundFrame = grabber->getFreshFrame(chrono::seconds(20));
					if (!backgroundFrame || !backgroundFrame->getPixels().isAllocated()) {
						throw(ofxRulr::Exception("Failed to capture incoming frame"));
					}
					const auto background = backgroundFrame->getPixels();
					scopedProcessBackground.end();

					//this happens anyway (setting the preview frame)
					this->previewFrame = this->findLine(foreground, background);

					scopedProcess.end();
				}
			}
		}
	}
}
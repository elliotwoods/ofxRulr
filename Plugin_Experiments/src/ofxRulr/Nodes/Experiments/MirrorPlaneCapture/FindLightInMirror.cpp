#include "pch_Plugin_Experiments.h"
#include "FindLightInMirror.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//-----------
				FindLightInMirror::FindLightInMirror()
				{
					RULR_NODE_INIT_LISTENER;
				}

				//-----------
				string
					FindLightInMirror::getTypeName() const
				{
					return "Halo::FindLightInMirror";
				}

				//-----------
				void
					FindLightInMirror::init()
				{
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;

					this->manageParameters(this->parameters);

					this->addInput<Item::Camera>();
					this->addInput<Heliostats2>(); // Used for debug only

					this->panel = make_shared<ofxCvGui::Panels::Groups::Grid>();
					{
						{
							auto panel = ofxCvGui::Panels::makeImage(this->preview.croppedImagePreview, "croppedImage");
							panel->onDrawImage += [this](ofxCvGui::DrawImageArguments& args) {
								ofPushStyle();
								{
									// Draw blob outline
									this->preview.blobOutline.draw();

									// Draw moments rect
									ofSetColor(0, 255, 0);
									ofNoFill();
									ofDrawRectangle(this->preview.momentsBounds);

									// Draw centroid
									ofSetColor(0, 0, 0);
									ofPushMatrix();
									{
										ofTranslate(this->preview.pointInCropped);
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);
									}
									ofPopMatrix();
								}
								ofPopStyle();
							};
							this->panel->add(panel);
						}

						this->panel->add(ofxCvGui::Panels::makeImage(this->preview.croppedMaskPreview, "croppedMask"));
						this->panel->add(ofxCvGui::Panels::makeImage(this->preview.maskedImagePreview, "maskedImage"));
						this->panel->add(ofxCvGui::Panels::makeImage(this->preview.backgroundPreview, "background"));
						this->panel->add(ofxCvGui::Panels::makeImage(this->preview.differencePreview, "difference"));
						
						{
							auto panel = ofxCvGui::Panels::makeImage(this->preview.binaryPreview, "binary");
							panel->onDrawImage += [this](ofxCvGui::DrawImageArguments& args) {
								ofPushStyle();
								{
									// Draw blob outline
									this->preview.blobOutline.draw();

									// Draw moments rect
									ofSetColor(0, 255, 0);
									ofNoFill();
									ofDrawRectangle(this->preview.momentsBounds);

									// Draw centroid
									ofSetColor(255, 0, 0);
									ofPushMatrix();
									{
										ofTranslate(this->preview.pointInCropped);
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);
									}
									ofPopMatrix();
								}
								ofPopStyle();
							};
							this->panel->add(panel);
						}
					}
				}

				//-----------
				void
					FindLightInMirror::update()
				{
					if (this->preview.dirty) {
						this->refreshPreview();
					}
				}

				//-----------
				void
					FindLightInMirror::populateInspector(ofxCvGui::InspectArguments& args)
				{
					auto inspector = args.inspector;

					inspector->addButton("Test find lights", [this]() {
						try {
							this->testFindLights();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, OF_KEY_RETURN)->setHeight(100.0f);
				}

				//-----------
				ofxCvGui::PanelPtr
					FindLightInMirror::getPanel()
				{
					return this->panel;
				}

				//-----------
				FindLightInMirror::Result
					FindLightInMirror::findLight(const cv::Mat& image
						, const ofxRay::Camera& cameraView
						, shared_ptr<Heliostats2> heliostatsNode
						, shared_ptr<Heliostats2::Heliostat> heliostat
						, const FindSettings & findSettings)
				{
					if (findSettings.buildPreview) {
						this->preview.dirty = true;
					}

					switch (this->parameters.mode.get()) {
					case Mode::NonInteractive:
					{
						return this->findLightRoutine(image
							, cameraView
							, heliostatsNode
							, heliostat
							, findSettings);
						break;
					}
					case Mode::Interactive:
					{
						return this->findLightInteractive(image
							, cameraView
							, heliostatsNode
							, heliostat
							, findSettings);
						break;
					}
					default:
						throw(ofxRulr::Exception("Not implemented"));
						break;
					}
				}
				
				//-----------
				vector<FindLightInMirror::Result>
					FindLightInMirror::findLights(const cv::Mat & image
						, vector<shared_ptr<Heliostats2::Heliostat>> heliostats
						, shared_ptr<Heliostats2> heliostatsNode
						, const FindSettings & findSettings)
				{
					this->throwIfMissingAConnection<Item::Camera>();
					auto cameraNode = this->getInput<Item::Camera>();

					// Get the camera view
					auto cameraView = cameraNode->getViewInWorldSpace();

					vector<Result> results;
					for (auto heliostat : heliostats) {
						try {
							auto result = this->findLight(image
								, cameraView
								, heliostatsNode
								, heliostat
								, findSettings);

							results.push_back(result);
						}
						catch(Exception& e) {
							ofLogWarning("FindLightInMirror") << e.what();
							Result result{
								false
								, glm::vec2()
								, e.what()
							};
							results.push_back(result);
						}
					}

					return results;
				}

				//----------
				struct InteractiveWindowData {
					glm::vec2 mouseInMirror{ 0 , 0 };
					int scaleUpSteps = 0;
					int imageWidth = 1;
					int imageHeight = 1;
					bool needsUpdate = true;
				};

				void
					onMouseInInterativeWindow(int event, int x, int y, int flags, void* userData)
				{
					auto& windowData = *(InteractiveWindowData*)userData;
					for (int i = 0; i < windowData.scaleUpSteps; i++) {
						x /= 2;
						y /= 2;
					}
					x %= windowData.imageWidth;

					windowData.mouseInMirror.x = ((float)x / (float)windowData.imageWidth) * 2.0f - 1.0f;
					windowData.mouseInMirror.y = ((float)y / (float)windowData.imageHeight) * 2.0f - 1.0f;

					windowData.needsUpdate = true;
				}

				//----------
				FindLightInMirror::Result
					FindLightInMirror::findLightInteractive(const cv::Mat& image
						, const ofxRay::Camera& cameraView
						, shared_ptr<Heliostats2> heliostatsNode
						, shared_ptr<Heliostats2::Heliostat> heliostat
						, const FindSettings & findSettings)
				{
					const auto windowName = "Interactive light finding";
					InteractiveWindowData windowData;

					cv::namedWindow(windowName, 1);
					cv::setMouseCallback(windowName, onMouseInInterativeWindow, &windowData);
					
					cv::Mat preview;
					Result result;

					while (true) {

						// Update the result
						if (windowData.needsUpdate) {
							auto findSettingsWithDebugViews = findSettings;
							findSettingsWithDebugViews.buildPreview = true;

							// Perform the find
							try {
								result = this->findLightRoutine(image
									, cameraView
									, heliostatsNode
									, heliostat
									, findSettings
									, windowData.mouseInMirror);
							}
							RULR_CATCH_ALL_TO_ERROR;

							// Create the preview images
							{
								cv::hconcat(this->preview.croppedImage, this->preview.binary, preview);
								windowData.imageWidth = this->preview.croppedImage.cols;
								windowData.imageHeight = this->preview.croppedImage.rows;

								for (int i = 0; i < windowData.scaleUpSteps; i++) {
									cv::pyrUp(preview, preview);
								}
								cv::cvtColor(preview, preview, cv::COLOR_GRAY2RGB);
							}

							// Draw the result
							if (result.success) {
								// Get the data
								const cv::Point2f centroid = ofxCv::toCv(this->preview.pointInCropped); // centroid in image space
								vector<cv::Point2i> contour;
								{
									const auto& vertices = this->preview.blobOutline.getVertices();
									for (const auto& vertex : vertices) {
										contour.push_back({ (int)vertex.x, (int)vertex.y });
									}
								}
								const cv::Rect bounds = ofxCv::toCv(this->preview.momentsBounds);
								
								auto previewScale = 1;
								for (int i = 0; i < windowData.scaleUpSteps; i++) {
									previewScale *= 2;
								}

								// Draw for 2 previews
								for(int i=0; i<2; i++) {
									int xOffset = i * windowData.imageWidth;

									// Centroid
									{
										// Draw the centroid as a cross
										int crossSize = 5; // Size of the cross arms
										cv::Scalar crossColor(0, 255, 0); // Color of the cross (Green in this case)
										int crossThickness = 2; // Thickness of the cross lines

										// Draw horizontal line of the cross
										cv::line(preview,
											cv::Point(centroid.x + xOffset - crossSize, centroid.y) * previewScale,
											cv::Point(centroid.x + xOffset + crossSize, centroid.y) * previewScale,
											crossColor,
											crossThickness);

										// Draw vertical line of the cross
										cv::line(preview,
											cv::Point(centroid.x + xOffset, centroid.y - crossSize) * previewScale,
											cv::Point(centroid.x + xOffset, centroid.y + crossSize) * previewScale,
											crossColor,
											crossThickness);
									}

									// Bounds
									{
										// Draw bounds as a rectangle
										cv::Scalar boundsColor(255, 0, 0); // Color of the rectangle (Blue in this case)
										int boundsThickness = 2; // Thickness of the rectangle lines

										auto boundsScaled = bounds;
										{
											boundsScaled.x = boundsScaled.x * previewScale + xOffset;
											boundsScaled.y *= previewScale;
											boundsScaled.width *= previewScale;
											boundsScaled.height *= previewScale;
										}

										cv::rectangle(preview, boundsScaled, boundsColor, boundsThickness);
									}

									// Draw contour as an outline
									{
										std::vector<std::vector<cv::Point>> contours = { contour }; // OpenCV contours need a vector of vector of points
										cv::Scalar contourColor(0, 0, 255); // Color of the contour (Red in this case)
										int contourThickness = 2; // Thickness of the contour line

										for (auto& element : contours) {
											for (auto& vertex : element) {
												vertex.x = vertex.x * previewScale + xOffset;
												vertex.y *= previewScale;
											}
										}
										cv::drawContours(preview, contours, -1, contourColor, contourThickness);
									}

									// Draw text annotations
									{
										int fontFace = cv::FONT_HERSHEY_SIMPLEX;
										double fontScale = 0.7;
										int thickness = 2;
										cv::Scalar textColor(0, 255, 0); // Green color

										// Set the starting position for the text
										int textX = 10; // 10 pixels from the left
										int textY = 30; // Start 30 pixels from the top and increment for each line
										int lineHeight = 30; // Line height for each text line

										// Draw each line of text
										cv::putText(preview, "Press [SPACE] to accept result", cv::Point(textX, textY), fontFace, fontScale, textColor, thickness);
										textY += lineHeight; // Move to the next line
										cv::putText(preview, "Press [ESC]/[BACKSPACE] to reject result", cv::Point(textX, textY), fontFace, fontScale, textColor, thickness);
										textY += lineHeight; // Move to the next line

										// Show the current threshold value
										std::string thresholdText = "Press [+]/[-] to change threshold (Current: " + std::to_string(this->parameters.detection.threshold.get()) + ")";
										cv::putText(preview, thresholdText, cv::Point(textX, textY), fontFace, fontScale, textColor, thickness);
									}
								}
							}

							windowData.needsUpdate = false;
						}

						// Show the preview
						cv::imshow(windowName, preview);

						// Handle keyboard inputs
						{
							char key = cv::waitKey(20);
							switch (key) {
							case 27: // Escape
							case 8:  // Backspace
								cv::destroyWindow(windowName);
								throw(ofxRulr::Exception("User cancelled the operation"));
							case '+':
							{
								auto threshold = this->parameters.detection.threshold.get();
								threshold += 4;
								if (threshold > this->parameters.detection.threshold.getMax()) {
									threshold = this->parameters.detection.threshold.getMax();
								}
								this->parameters.detection.threshold.set(threshold);
								windowData.needsUpdate = true;
								break;
							}
							case '-':
							{
								auto threshold = this->parameters.detection.threshold.get();
								threshold -= 4;
								if (threshold < this->parameters.detection.threshold.getMin()) {
									threshold = this->parameters.detection.threshold.getMin();
								}
								this->parameters.detection.threshold.set(threshold);
								windowData.needsUpdate = true;
								break;
							}
							case ' ':
							{
								// OK! let's go!
								cv::destroyWindow(windowName);
								return result;
							}
							}
						}
					}
				}

				//----------
				FindLightInMirror::Result
					FindLightInMirror::findLightRoutine(const cv::Mat& image
					, const ofxRay::Camera& cameraView
					, shared_ptr<Heliostats2> heliostatsNode
					, shared_ptr<Heliostats2::Heliostat> heliostat
					, const FindSettings& findSettings
					, const glm::vec2& closestToInMirror)
				{
					auto mask = heliostatsNode->drawMirrorFaceMask(heliostat
						, cameraView
						, this->parameters.mask.scale.get());

					// Get the bounding box of the mask
					cv::Rect cropBoundingBox;
					{
						cv::Mat activePixels;
						cv::findNonZero(mask, activePixels);
						cropBoundingBox = cv::boundingRect(activePixels);
					}

					// Check if the remaining mirror is large enough to consider
					{
						const auto& minDimension = this->parameters.mask.minDimension.get();
						if (cropBoundingBox.width < minDimension
							|| cropBoundingBox.height < minDimension) {
							throw(ofxRulr::Exception("Heliostat in camera is smaller than min dimension"));
						}
					}

					// Crop the image
					auto croppedImage = image(cropBoundingBox);
					auto croppedMask = mask(cropBoundingBox);

					// Mask the cropped image
					cv::Mat maskedImage = croppedImage.clone();
					if (this->parameters.mask.enabled.get()) {
						// Get mean pixel value for fill
						auto meanValue = cv::mean(croppedImage, croppedMask);

						// Set all pixels outside of this to 0
						cv::Mat invertedMask;
						cv::bitwise_not(croppedMask, invertedMask);
						maskedImage.setTo(cv::Scalar(meanValue), invertedMask);

						if (findSettings.buildPreview) {
							this->preview.maskedImage = maskedImage;
						}
					}

					if (findSettings.buildPreview) {
						this->preview.croppedImage = croppedImage;
						this->preview.croppedMask = croppedMask;
						this->preview.maskedImage = maskedImage;
					}

					// Create a background
					cv::Mat background;
					{
						auto blur = (int)(this->parameters.detection.blur.get() * (float)maskedImage.cols);
						if (blur <= 0) {
							throw(ofxRulr::Exception("Background blur is too small"));
						}

						if (this->parameters.detection.blurIterative) {
							cout << blur << endl;

							// Iteratively blur for speed
							background = maskedImage;
							const int blurKernelSize = 5;
							auto blurIterations = (int)blur / blurKernelSize;
							cout << blurIterations << endl;

							for (int i = 0; i < blur / blurKernelSize; i++) {
								cv::blur(background
									, background
									, cv::Size(blurKernelSize, blurKernelSize));
							}

							if (findSettings.buildPreview) {
								this->preview.background = background;
							}
						}
						else {
							auto blurSize = (int)(blur / 2) * 2 + 1;
							cv::blur(maskedImage
								, background
								, cv::Size(blur, blur));
						}
					}

					// Subtract the image from the background
					cv::Mat difference;
					{
						cv::subtract(maskedImage, background, difference);

						if (findSettings.buildPreview) {
							this->preview.difference = difference;
						}
					}

					// Take a threshold against the difference image
					cv::Mat binary;
					{
						cv::threshold(difference
							, binary
							, this->parameters.detection.threshold.get()
							, 255
							, cv::THRESH_BINARY);

						if (findSettings.buildPreview) {
							this->preview.binary = binary;
						}
					}

					// Remove noise (small high values)
					{
						auto element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

						// Erode
						for (int i = 0; i < this->parameters.detection.removeNoise.get(); i++) {
							cv::erode(binary, binary, element);
						}

						// Dilate
						for (int i = 0; i < this->parameters.detection.removeNoise.get(); i++) {
							cv::dilate(binary, binary, element);
						}
					}

					// Find the centroid of the feature
					glm::vec2 centroid;
					{
						// Take contours
						vector<vector<cv::Point2i>> contours;
						cv::findContours(binary
							, contours
							, cv::RETR_EXTERNAL
							, cv::CHAIN_APPROX_NONE);

						if (contours.empty()) {
							throw(ofxRulr::Exception("Failed to find any contours"));
						}

						// Get target position
						glm::vec2 targetPosition;
						{
							targetPosition.x = (closestToInMirror.x + 1.0f) / 2.0f * (float)binary.cols;
							targetPosition.y = (closestToInMirror.y + 1.0f) / 2.0f * (float)binary.rows;
						}

						// Select favourite contour
						auto maxArea = (int)(this->parameters.detection.maxArea.get() * (float)binary.cols * (float)binary.rows);
						int selectedFeatureIndex = -1;
						float bestDistanceToCenterSoFar = binary.cols;
						for (int i = 0; i < contours.size(); i++) {
							const auto& contour = contours[i];
							auto bounds = cv::boundingRect(contour);
							auto area = bounds.area();

							// Ignore large contours
							if (area > maxArea) {
								continue;
							}

							glm::vec2 blobCenter(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);

							auto distanceToCenter = glm::length(targetPosition - blobCenter);

							// Take the largest remaining area contour
							if (distanceToCenter < bestDistanceToCenterSoFar) {
								selectedFeatureIndex = i;
								bestDistanceToCenterSoFar = distanceToCenter;
							}
						}

						// No favourite found
						if (selectedFeatureIndex == -1) {
							throw(ofxRulr::Exception("All contours failed"));
						}

						auto contour = contours[selectedFeatureIndex];

						if (findSettings.buildPreview) {
							this->preview.blobOutline.clear();
							for (const auto& vertex : contour) {
								this->preview.blobOutline.addVertex({ vertex.x, vertex.y, 0 });
							}
							this->preview.blobOutline.close();
						}

						// Dilate a rect around the found contour for finding moments
						cv::Rect2i dilatedRect = cv::boundingRect(contour);
						{
							const auto& dilationForMoments = this->parameters.detection.dilateForMoments.get();

							dilatedRect.x -= dilationForMoments;
							dilatedRect.y -= dilationForMoments;
							dilatedRect.width += dilationForMoments;
							dilatedRect.height += dilationForMoments;
						}

						if (findSettings.buildPreview) {
							this->preview.momentsBounds = ofxCv::toOf(dilatedRect);
						}

						// Take moments centroid
						auto moment = cv::moments(difference(dilatedRect));
						centroid = glm::vec2(moment.m10 / moment.m00 + dilatedRect.x
							, moment.m01 / moment.m00 + dilatedRect.y);

						if (findSettings.buildPreview) {
							this->preview.pointInCropped = centroid;
						}
					}

					// Offset by cropped bounding box
					centroid.x += cropBoundingBox.x;
					centroid.y += cropBoundingBox.y;

					return Result{
						true
						, centroid
					};
				}

				//-----------
				void
					FindLightInMirror::testFindLights()
				{
					this->throwIfMissingAnyConnection();
					
					auto heliostatsNode = this->getInput<Heliostats2>();
					auto heliostats = heliostatsNode->getHeliostats();

					if (heliostats.empty()) {
						throw(ofxRulr::Exception("No heliostats selected"));
					}
					auto heliostat = heliostats.front();

					auto fileResult = ofSystemLoadDialog("Select image file");
					if (!fileResult.bSuccess) {
						throw(ofxRulr::Exception("No file selected"));
					}

					auto imageFromFile = cv::imread(fileResult.filePath);
					if (imageFromFile.empty()) {
						throw(ofxRulr::Exception("Image couldn't open"));
					}

					cv::Mat image;
					{
						if (imageFromFile.channels() > 1) {
							cv::cvtColor(imageFromFile, image, cv::COLOR_RGB2GRAY);
						}
						else {
							image = imageFromFile;
						}
					}

					FindSettings findSettings;
					{
						findSettings.buildPreview = true;
					}

					auto cameraNode = this->getInput<Item::Camera>();
					auto cameraView = cameraNode->getViewInWorldSpace();

					this->findLight(image
						, cameraView
						, heliostatsNode
						, heliostat
						, findSettings);
				}

				//-----------
				void
					FindLightInMirror::refreshPreview()
				{
					ofxCv::copy(this->preview.croppedImage, this->preview.croppedImagePreview);
					ofxCv::copy(this->preview.croppedMask, this->preview.croppedMaskPreview);
					ofxCv::copy(this->preview.maskedImage, this->preview.maskedImagePreview);
					ofxCv::copy(this->preview.background, this->preview.backgroundPreview);
					ofxCv::copy(this->preview.difference, this->preview.differencePreview);
					ofxCv::copy(this->preview.binary, this->preview.binaryPreview);
					
					this->preview.croppedImagePreview.update();
					this->preview.croppedMaskPreview.update();
					this->preview.maskedImagePreview.update();
					this->preview.backgroundPreview.update();
					this->preview.differencePreview.update();
					this->preview.binaryPreview.update();

					this->preview.dirty = false;
				}
			}
		}
	}
}
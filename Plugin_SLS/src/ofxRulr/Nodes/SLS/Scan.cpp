#include "pch_Plugin_SLS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace SLS {
			//----------
			Scan::Scan() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string Scan::getTypeName() const {
				return "SLS::Scan";
			}

			//----------
			void Scan::init() {
				this->addInput<Item::Camera>();
				this->addInput<System::VideoOutput>();
			}

			//----------
			shared_ptr<Scan::DataSet> Scan::capture() {
				this->throwIfMissingAnyConnection();

				auto camera = this->getInput<Item::Camera>();
				auto videoOutput = this->getInput<System::VideoOutput>();

				auto grabber = camera->getGrabber();
				if (!grabber) {
					throw(ofxRulr::Exception("Cannot scan without a grabber"));
				}

				auto suite = this->getSuite();
				auto dataSet = make_shared<DataSet>();
				dataSet->cameraWidth = (int)grabber->getWidth();
				dataSet->cameraHeight = (int)grabber->getHeight();

				//capture the projected scan patterns
				{
					//clear the output
					this->message.allocate(suite->params.width
						, suite->params.height
						, ofImageType::OF_IMAGE_GRAYSCALE);

					ofHideCursor();

					try {
						auto patternCount = suite->pattern->getNumberOfPatternImages();
						auto totalFrameCount = patternCount + 2;
						Utils::ScopedProcess scopedProcess("Scanning graycode"
							, true
							, totalFrameCount);

						for (int iFrame = 0; iFrame < totalFrameCount; iFrame++) {
							Utils::ScopedProcess frameScopedProcess("Scanning frame", false);

							cv::Mat * input;
							cv::Mat * output;
							
							if (iFrame == 0) {
								//white
								input = &suite->whiteProjector;
								output = &dataSet->whiteCamera;
							}
							else if (iFrame == 1) {
								//black
								input = &suite->blackProjector;
								output = &dataSet->blackCamera;
							}
							else {
								//data frame
								input = &suite->patternImages[iFrame - 2];
								output = &dataSet->captures[iFrame - 2];
							}

							//copy image to message texture
							{
								ofxCv::copy(*input, message.getPixels());
								message.update();
							}
							
#ifdef TARGET_OSX
							//see notes on Graycode node
							for (int iOSXRedo = 0; iOSXRedo < 2; iOSXRedo++) {
#endif
								for (int iFlush = 0; iFlush < this->parameters.scan.flushOutputFrames + 1; iFlush++) {
									videoOutput->clearFbo(false);
									videoOutput->begin();
									{
										ofPushStyle();
										{
											auto brightness = this->parameters.scan.brightness;
											ofSetColor(brightness * 255.0f);
											this->message.draw(0, 0);
										}
										ofPopStyle();
									}
									videoOutput->end();
									videoOutput->presentFbo();
								}

								auto startWait = ofGetElapsedTimeMillis();
								while (ofGetElapsedTimeMillis() - startWait < this->parameters.scan.captureDelay) {
									ofSleepMillis(1);
									grabber->update();
								}

								for (int iFlush = 0; iFlush < this->parameters.scan.flushInputFrames; iFlush++) {
									grabber->getFreshFrame();
								}
#ifdef TARGET_OSX
							}
#endif
							auto frame = grabber->getFreshFrame();
							if (!frame) {
								throw(ofxRulr::Exception("Couldn't get fresh frame from camera"));
							}
							ofxCv::copy(frame->getPixels(), *output);
						}
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT
						catch (...) {
					}
					ofShowCursor();
				}

				dataSet->suite = suite;
				return dataSet;
			}

			//----------
			void Scan::decode(shared_ptr<DataSet> dataSet) {
				//decode the scan
				{
					auto dataSet = make_shared<DataSet>();

					//calculate projector pixel indices
					{
						dataSet->projectorXYInCamera.allocate(dataSet->cameraWidth
							, dataSet->cameraHeight
							, 2);

						for (int j = 0; j < dataSet->cameraHeight; j++) {
							auto line = dataSet->projectorXYInCamera.getLine(j);
							auto output = line.begin();
							for (int i = 0; i < dataSet->cameraWidth; i++) {
								cv::Point2i projectorPixel;
								dataSet->suite->pattern->getProjPixel(
									dataSet->captures
									, i
									, j
									, projectorPixel);
								*output++ = projectorPixel.x;
								*output++ = projectorPixel.y;
							}
						}
					}

					//calculate the mask
					{
						dataSet->mask.allocate(dataSet->cameraWidth
							, dataSet->cameraHeight
							, OF_IMAGE_GRAYSCALE);

						int whiteThreshold = this->parameters.threshold.white;
						int blackThreshold = this->parameters.threshold.black;

						//from https://github.com/opencv/opencv_contrib/blob/master/modules/structured_light/src/graycodepattern.cpp#L344

						auto output = (uint8_t*)dataSet->mask.getData();
						auto white = (uint8_t*)dataSet->whiteCamera.data;
						auto black = (uint8_t*)dataSet->blackCamera.data;

						size_t size = dataSet->cameraWidth * dataSet->cameraHeight;
						for (size_t i = 0; i < size; i++) {
							*output++ = *white++ - *black++ > blackThreshold;
						}
					}
				}
			}

			//----------
			void Scan::scan() {
				auto dataSet = this->capture();
				decode(dataSet);
			}

			//----------
			shared_ptr<Scan::Suite> Scan::getSuite() {
				this->throwIfMissingAConnection<System::VideoOutput>();
				auto videoOutput = this->getInput<System::VideoOutput>();
				if (!videoOutput->isWindowOpen()) {
					throw(ofxRulr::Exception(""));
				}
				

				auto suite = make_shared<Suite>();
				suite->params.width = videoOutput->getWidth();
				suite->params.height = videoOutput->getHeight();
				
				auto graycode = cv::structured_light::GrayCodePattern::create(suite->params);
				graycode->generate(suite->patternImages);
				graycode->getImagesForShadowMasks(suite->blackProjector, suite->whiteProjector);

				return suite;
			}
		}
	}
}
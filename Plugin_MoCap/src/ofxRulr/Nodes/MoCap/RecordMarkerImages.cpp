#include "pch_Plugin_MoCap.h"
#include "RecordMarkerImages.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			//----------
			RecordMarkerImages::RecordMarkerImages() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string RecordMarkerImages::getTypeName() const {
				return "MoCap::RecordMarkerImages";
			}

			//----------
			void RecordMarkerImages::init() {
				this->onDeserialize += [this](const nlohmann::json &) {
					this->parameters.recording.enabled = false;
				};

				this->threadPool = make_unique<Utils::ThreadPool>(4, 100);

				this->manageParameters(this->parameters);
			}

			//----------
			void RecordMarkerImages::processFrame(shared_ptr<ofxMachineVision::Frame> incomingFrame) {
				auto outgoingFrame = make_shared<RecordMarkerImagesFrame>();
				outgoingFrame->incomingFrame = incomingFrame;
				outgoingFrame->image = ofxCv::toCv(incomingFrame->getPixels());			

				//local difference
				{
					//iterative blur
					{
						int blurSize = this->parameters.localDifference.blurSize;

						cv::blur(outgoingFrame->image, outgoingFrame->blurred, cv::Size(blurSize / 2, blurSize / 2));
						blurSize /= 2;
						while (blurSize > 1) {
							if (blurSize <= 32) {
								cv::blur(outgoingFrame->blurred, outgoingFrame->blurred, cv::Size(blurSize, blurSize));
								break;
							}
							cv::blur(outgoingFrame->blurred, outgoingFrame->blurred, cv::Size(blurSize / 2, blurSize / 2));
							blurSize /= 2;
						}
					}

					outgoingFrame->difference = outgoingFrame->image - outgoingFrame->blurred;

					cv::threshold(outgoingFrame->difference
						, outgoingFrame->binary
						, this->parameters.localDifference.threshold
						, 255
						, cv::THRESH_BINARY);
				}
				
				//contour and bounding boxes
				{
					cv::findContours(outgoingFrame->binary
						, outgoingFrame->contours
						, cv::RETR_EXTERNAL
						, cv::CHAIN_APPROX_NONE);

					auto minimumArea = this->parameters.contourFilter.minimumArea.get();
					minimumArea *= minimumArea;
					auto maximumArea = this->parameters.contourFilter.maximumArea.get();
					maximumArea *= maximumArea;
					for (const auto & contour : outgoingFrame->contours) {
						const auto area = cv::contourArea(contour);
						if (area >= minimumArea && area <= maximumArea) {
							outgoingFrame->filteredContours.push_back(contour);
							outgoingFrame->boundingBoxes.emplace_back(cv::boundingRect(contour));
						}
					}
				}

				//recording
				if (this->parameters.recording.enabled) {
					auto dataRoot = ofToDataPath(".", true);
					
					//make the upper folder
					auto folderPath = std::filesystem::path(dataRoot)
						/ this->getDefaultFilename();
					filesystem::create_directory(folderPath);

					auto filePath = folderPath / ofToString(incomingFrame->getTimestamp().count());

					cv::imwrite(filePath.string() + ".png", outgoingFrame->image);

					{
						cv::FileStorage fs(filePath.string() + ".yml", cv::FileStorage::WRITE);
						fs << "boundingBoxes" << outgoingFrame->boundingBoxes;
						fs << "contours" << outgoingFrame->contours;
					}
				}
				this->onNewFrame(outgoingFrame);
			}

			//----------
			size_t RecordMarkerImages::getThreadPoolSize() const {
				return thread::hardware_concurrency() - 1;
			}

			//----------
			size_t RecordMarkerImages::getThreadPoolQueueSize() const {
				return 100;
			}
		}
	}
}
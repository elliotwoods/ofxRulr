#include "pch_Plugin_MoCap.h"
#include "FindMarkerCentroids.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			//----------
			FindMarkerCentroids::FindMarkerCentroids() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string FindMarkerCentroids::getTypeName() const {
				return "MoCap::FindMarkerCentroids";
			}

			//----------
			void FindMarkerCentroids::init() {
				this->manageParameters(this->parameters);
			}

			//----------
			void FindMarkerCentroids::processFrame(shared_ptr<ofxMachineVision::Frame> incomingFrame) {
				//create the ouput frame;
				auto outgoingFrame = make_shared<FindMarkerCentroidsFrame>();
				outgoingFrame->imageFrame = incomingFrame; 

				//convert to grayscale if needs be
				outgoingFrame->image = ofxCv::toCv(incomingFrame->getPixels());
				switch (incomingFrame->getPixels().getPixelFormat()) {
				case ofPixelFormat::OF_PIXELS_GRAY:
					break;
				case ofPixelFormat::OF_PIXELS_RGB:
				case ofPixelFormat::OF_PIXELS_BGR:
					cv::cvtColor(outgoingFrame->image, outgoingFrame->image, cv::COLOR_RGB2GRAY);
					break;
				case ofPixelFormat::OF_PIXELS_RGBA:
				case ofPixelFormat::OF_PIXELS_BGRA:
					cv::cvtColor(outgoingFrame->image, outgoingFrame->image, cv::COLOR_RGBA2GRAY);
					break;
				default:
					throw(ofxRulr::Exception("Image format not supported by FindContourMarkers"));
				}

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
					outgoingFrame->difference *= this->parameters.localDifference.differenceAmplify;

					cv::threshold(outgoingFrame->difference
						, outgoingFrame->binary
						, this->parameters.localDifference.threshold
						, 255
						, cv::THRESH_BINARY);
				}

				//find the contours
				cv::findContours(outgoingFrame->binary
					, outgoingFrame->contours
					, cv::RETR_EXTERNAL
					, cv::CHAIN_APPROX_NONE);

				auto count = outgoingFrame->contours.size();

				//find the bounding rectangles (check if valid also)
				outgoingFrame->boundingRects.reserve(count);
				for (const auto & contour : outgoingFrame->contours) {
					auto rect = cv::boundingRect(contour);

					//check area
					if (rect.area() <= this->parameters.contourFilter.minimumArea) {
						continue;
					}

					//check if it touches edge of frame (we use a threshold of 2px for rejections)
					{
						const int distanceThreshold = 2;
						auto bottomRight = rect.br();
						if (rect.x <= distanceThreshold
							|| rect.y <= distanceThreshold
							|| outgoingFrame->image.cols - bottomRight.x <= distanceThreshold
							|| outgoingFrame->image.rows - bottomRight.y <= distanceThreshold) {
							continue;
						}
					}

					//create a dilated rect for finding moments
					auto dilatedRect = rect;
					{
						dilatedRect.x -= outgoingFrame->dilationSize;
						dilatedRect.y -= outgoingFrame->dilationSize;
						dilatedRect.width += outgoingFrame->dilationSize;
						dilatedRect.height += outgoingFrame->dilationSize;
					}
					auto moment = cv::moments(outgoingFrame->image(dilatedRect));

					//check circularity
					//https://github.com/opencv/opencv/blob/master/modules/features2d/src/blobdetector.cpp#L225
					float circularity;
					{
						auto area = moment.m00;
						auto perimeter = cv::arcLength(cv::Mat(contour), true);
						circularity = 4 * CV_PI * area / (perimeter * perimeter) / pow(max(rect.width, rect.height), this->parameters.contourFilter.circularityGamma.get());
						if (circularity < this->parameters.contourFilter.minimumCircularity.get()) {
							continue;
						}
					}
					
					outgoingFrame->boundingRects.push_back(rect);
					outgoingFrame->moments.push_back(moment);
					outgoingFrame->circularity.push_back(circularity);
				}
				count = outgoingFrame->boundingRects.size();

				//get moments centers
				outgoingFrame->centroids.reserve(count);
				for (size_t i = 0; i < count; i++) {
					const auto & moment = outgoingFrame->moments[i];
					outgoingFrame->centroids.emplace_back(
						moment.m10 / moment.m00 + outgoingFrame->boundingRects[i].x - outgoingFrame->dilationSize
						, moment.m01 / moment.m00 + outgoingFrame->boundingRects[i].y - outgoingFrame->dilationSize
					);
				}

				//announce the new frame
				this->onNewFrame(outgoingFrame);
			}
		}
	}
}
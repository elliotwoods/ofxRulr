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
				auto & pixels = incomingFrame->getPixels();

				//convert to grayscale if needs be
				cv::Mat image = ofxCv::toCv(pixels);
				switch (pixels.getPixelFormat()) {
				case ofPixelFormat::OF_PIXELS_GRAY:
					break;
				case ofPixelFormat::OF_PIXELS_RGB:
				case ofPixelFormat::OF_PIXELS_BGR:
					cv::cvtColor(image, image, CV_RGB2GRAY);
					break;
				case ofPixelFormat::OF_PIXELS_RGBA:
				case ofPixelFormat::OF_PIXELS_BGRA:
					cv::cvtColor(image, image, CV_RGBA2GRAY);
					break;
				default:
					throw(ofxRulr::Exception("Image format not supported by FindContourMarkers"));
				}

				//create the ouput frame;
				auto outgoingFrame = make_shared<FindMarkerCentroidsFrame>();
				outgoingFrame->imageFrame = incomingFrame;

				//apply the threshold
				cv::threshold(image
					, outgoingFrame->binaryImage
					, this->parameters.threshold.get()
					, 255
					, cv::THRESH_BINARY);

				//find the contours
				cv::findContours(outgoingFrame->binaryImage
					, outgoingFrame->contours
					, CV_RETR_EXTERNAL
					, CV_CHAIN_APPROX_NONE);

				auto count = outgoingFrame->contours.size();

				//find the bounding rectangles (check if valid also)
				outgoingFrame->boundingRects.reserve(count);
				for (const auto & contour : outgoingFrame->contours) {
					auto rect = cv::boundingRect(contour);

					//check area
					if (rect.area() <= this->parameters.minimumArea) {
						continue;
					}

					//check if it touches edge of frame (we use a threshold of 2px for rejections)
					{
						const int distanceThreshold = 2;
						auto bottomRight = rect.br();
						if (rect.x <= distanceThreshold
							|| rect.y <= distanceThreshold
							|| image.cols - bottomRight.x <= distanceThreshold
							|| image.rows - bottomRight.y <= distanceThreshold) {
							continue;
						}
					}

					auto moment = cv::moments(image(rect));

					//check circularity
					//https://github.com/opencv/opencv/blob/master/modules/features2d/src/blobdetector.cpp#L225
					float circularity;
					{
						auto area = moment.m00;
						auto perimeter = cv::arcLength(cv::Mat(contour), true);
						circularity = 4 * CV_PI * area / (perimeter * perimeter) / pow(max(rect.width, rect.height), this->parameters.circularityGamma.get());
						if (circularity < this->parameters.minimumCircularity.get()) {
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
					outgoingFrame->centroids.emplace_back(moment.m10 / moment.m00 + outgoingFrame->boundingRects[i].x
						, moment.m01 / moment.m00 + outgoingFrame->boundingRects[i].y);
				}

				//announce the new frame
				this->onNewFrame(outgoingFrame);
			}
		}
	}
}
#pragma once

#include "ThreadedProcessNode.h"
#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			struct RecordMarkerImagesFrame {
				shared_ptr<ofxMachineVision::Frame> incomingFrame;

				cv::Mat image;
				cv::Mat blurred;
				cv::Mat difference;
				cv::Mat binary;

				vector<vector<cv::Point2i>> contours;
				vector<vector<cv::Point2i>> filteredContours;
				vector<cv::Rect> boundingBoxes;
			};

			class RecordMarkerImages : public ThreadedProcessNode<Item::Camera
				, ofxMachineVision::Frame
				, RecordMarkerImagesFrame> {
			public:
				RecordMarkerImages();
				virtual string getTypeName() const override;
				void init();
			protected:
				void processFrame(shared_ptr<ofxMachineVision::Frame>) override;

				size_t getThreadPoolSize() const override;
				size_t getThreadPoolQueueSize() const override;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> blurSize{ "Blur size", 20, 0, 1000 };
						ofParameter<float> threshold{ "Threshold", 50, 0, 255 };
						PARAM_DECLARE("Local difference", blurSize, threshold);
					} localDifference;

					struct : ofParameterGroup {
						ofParameter<float> minimumArea{ "Minimum area^0.5 [px]", 10, 0, 1000 };
						ofParameter<float> maximumArea{ "Maximum area^0.5 [px]", 200, 0, 1000 };
						PARAM_DECLARE("Contour filter", minimumArea, maximumArea);
					} contourFilter;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						PARAM_DECLARE("Recording", enabled);
					} recording;

					PARAM_DECLARE("RecordMarkerImages", localDifference, contourFilter, recording);
				} parameters;

				unique_ptr<Utils::ThreadPool> threadPool;
			};
		}
	}
}
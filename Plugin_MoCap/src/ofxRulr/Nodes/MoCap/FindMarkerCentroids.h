#pragma once

#include "ThreadedProcessNode.h"
#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			struct FindMarkerCentroidsFrame {
				shared_ptr<ofxMachineVision::Frame> imageFrame;
				cv::Mat binaryImage;
				vector<vector<cv::Point2i>> contours;
				vector<cv::Rect> boundingRects;
				vector<cv::Moments> moments;
				vector<float> circularity;
				vector<cv::Point2f> centroids;
			};

			class FindMarkerCentroids : public ThreadedProcessNode<Item::Camera
				, ofxMachineVision::Frame
				, FindMarkerCentroidsFrame> {
			public:
				FindMarkerCentroids();
				virtual string getTypeName() const override;
				void init();
			protected:
				void processFrame(shared_ptr<ofxMachineVision::Frame>) override;

				struct : ofParameterGroup {
					ofParameter<float> threshold{ "Threshold", 50, 0, 255 };
					ofParameter<float> minimumArea{ "Minimum area [px]", 100, 0, 10000 };
					ofParameter<float> circularityGamma{ "Circularity gamma", 0.8, 0, 2.0f };
					ofParameter<float> minimumCircularity{ "Minimum circularity ", 10.0, 0, 100.0f };
					PARAM_DECLARE("FindMarkerCentroids", threshold, minimumArea, circularityGamma, minimumCircularity);
				} parameters;
			};
		}
	}
}
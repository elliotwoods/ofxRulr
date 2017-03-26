#pragma once

#include "ThreadedProcessNode.h"
#include "FindMarkerCentroids.h"
#include "Body.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			struct CameraDescription {
				cv::Mat cameraMatrix;
				cv::Mat distortionCoefficients;
				cv::Mat rotationVector;
				cv::Mat translation;
			};

			struct MatchMarkersFrame {
				shared_ptr<FindMarkerCentroidsFrame> incomingFrame;
				shared_ptr<Body::Description> bodyDescription;
				shared_ptr<CameraDescription> cameraDescription;

				cv::Mat modelViewRotationVector;
				cv::Mat modelViewTranslation;

				vector<cv::Point3f> objectSpacePoints; // note this is empty when doing exhaustive search 
				vector<cv::Point2f> projectedMarkerImagePoints; // note this is empty when doing exhaustive search
				float distanceThresholdSquared; // note this flips to the exhaustive search parameter empty when doing exhaustive search

				size_t matchCount = 0;
				vector<size_t> matchedMarkerListIndex; // index in marker points list coming from Body
				vector<size_t> matchedMarkerID;
				vector<cv::Point2f> matchedProjectedPoint;
				vector<cv::Point2f> matchedCentroids;
				vector<cv::Point3f> matchedObjectSpacePoints;
			};

			class MatchMarkers : public ThreadedProcessNode<FindMarkerCentroids
			, FindMarkerCentroidsFrame
			, MatchMarkersFrame> {
			public:
				MatchMarkers();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments &);

				void performSearch();
			protected:
				void processFrame(shared_ptr<FindMarkerCentroidsFrame>) override;
				void processFullSearch(shared_ptr<MatchMarkersFrame> &);
				void processTrackingSearch(shared_ptr<MatchMarkersFrame> &);

				struct : ofParameterGroup {
					ofParameter<float> trackingDistanceThreshold{ "Tracking distance threshold [px]", 20, 0, 300 };
					ofParameter<float> searchDistanceThreshold{ "Search distance threshold [px]", 1, 0, 10 };
					ofParameter<bool> searchWhenTrackingLost{ "Search when tracking lost", false };
					PARAM_DECLARE("MatchMarkers", trackingDistanceThreshold, searchDistanceThreshold, searchWhenTrackingLost);
				} parameters;

				shared_ptr<Body::Description> bodyDescription;
				shared_ptr<CameraDescription> cameraDescription;
				mutex descriptionMutex;

				atomic<bool> needsFullSearch = false;
			};
		}
	}
}
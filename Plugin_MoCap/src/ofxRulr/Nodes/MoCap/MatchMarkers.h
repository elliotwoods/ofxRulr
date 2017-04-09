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

				ofMatrix4x4 viewProjectionMatrix;
			};

			struct MatchMarkersFrame {
				shared_ptr<FindMarkerCentroidsFrame> incomingFrame;
				shared_ptr<Body::Description> bodyDescription;
				shared_ptr<CameraDescription> cameraDescription;

				cv::Mat modelViewRotationVector;
				cv::Mat modelViewTranslation;

				struct {
					size_t count;
					vector<MarkerID> markerIDs;
					vector<cv::Point3f> objectSpacePoints; // note this is empty when doing exhaustive search 
					vector<cv::Point2f> projectedMarkerImagePoints; // note this is empty when doing exhaustive search
				} search;
				

				float distanceThresholdSquared; // note this flips to the exhaustive search parameter empty when doing exhaustive search

				bool trackingWasLost = false;

				struct {
					size_t count = 0;
					vector<size_t> markerListIndicies; // index in marker points list coming from Body
					vector<size_t> markerIDs;
					vector<cv::Point2f> projectedPoints;
					vector<cv::Point2f> centroids;
					vector<cv::Point3f> objectSpacePoints;
				} result;
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
					ofParameter<float> searchDistanceThreshold{ "Search distance threshold [px]", 5, 0, 300 };
					ofParameter<bool> searchWhenTrackingLost{ "Search when tracking lost", false };
					PARAM_DECLARE("MatchMarkers", trackingDistanceThreshold, searchDistanceThreshold, searchWhenTrackingLost);
				} parameters;

				shared_ptr<Body::Description> bodyDescription;
				shared_ptr<CameraDescription> cameraDescription;
				mutex descriptionMutex;

				atomic<bool> needsFullSearch = false;
				atomic<bool> searchInProgress = false;
			};
		}
	}
}
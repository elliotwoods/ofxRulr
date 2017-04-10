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
				cv::Mat inverseRotationVector;
				cv::Mat inverseTranslation;

				ofMatrix4x4 viewProjectionMatrix;
			};

			struct MatchMarkersFrame {
				shared_ptr<FindMarkerCentroidsFrame> incomingFrame;
				shared_ptr<Body::Description> bodyDescription;
				shared_ptr<CameraDescription> cameraDescription;

				cv::Mat modelViewRotationVector;
				cv::Mat modelViewTranslation;

				struct Search {
					size_t count;
					vector<MarkerID> markerIDs;
					vector<cv::Point3f> objectSpacePoints;
					vector<cv::Point2f> projectedMarkerImagePoints;
				} search;

				float distanceThresholdSquared;

				bool trackingWasLost = false;

				struct Result {
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
				//The captures define positions which the camera is likely to find itself in
				//We check these when we loose tracking
				class Capture : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Capture();
					string getDisplayString() const override;
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					cv::Point3d modelViewRotationVector;
					cv::Point3d modelViewTranslation;
				};
				
				MatchMarkers();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
			protected:
				void processFrame(shared_ptr<FindMarkerCentroidsFrame>) override;
				void processTrackingSearch(shared_ptr<MatchMarkersFrame> &);
				void processCheckKnownPoses(shared_ptr<MatchMarkersFrame> &);
				bool processModelViewTransform(shared_ptr<MatchMarkersFrame> &);

				struct : ofParameterGroup {
					ofParameter<float> trackingDistanceThreshold{ "Tracking distance threshold [px]", 20, 0, 300 };
					PARAM_DECLARE("MatchMarkers", trackingDistanceThreshold);
				} parameters;

				Utils::CaptureSet<Capture> captures;

				shared_ptr<Body::Description> bodyDescription;
				shared_ptr<CameraDescription> cameraDescription;
				mutex descriptionMutex;

				atomic<bool> needsTakeCapture = false;
				ofThreadChannel<shared_ptr<Capture>> newCaptures;
			};
		}
	}
}
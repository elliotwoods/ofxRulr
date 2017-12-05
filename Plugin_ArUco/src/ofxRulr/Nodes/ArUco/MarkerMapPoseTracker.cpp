#include "pch_Plugin_ArUco.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			MarkerMapPoseTracker::MarkerMapPoseTracker() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string MarkerMapPoseTracker::getTypeName() const {
				return "ArUco::MarkerMapPoseTracker";
			}

			//----------
			void MarkerMapPoseTracker::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::Camera>();
				this->addInput<MarkerMap>();
				this->addInput<Detector>();

				this->manageParameters(this->parameters);
			}
			aruco::MarkerMapPoseTracker markerMapPoseTracker;

			//----------
			void MarkerMapPoseTracker::update() {
				try {
					if (this->parameters.onNewFrame) {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							if (camera->getGrabber()->isFrameNew()) {
								this->track();
							}
						}
					}
				}
				RULR_CATCH_ALL_TO_ERROR;
			}

			//----------
			void MarkerMapPoseTracker::track() {
				auto cameraNode = this->getInput<Item::Camera>();
				auto detectorNode = this->getInput<Detector>();
				auto markerMapNode = this->getInput<MarkerMap>();

				if (cameraNode && detectorNode && markerMapNode) {
					auto grabber = cameraNode->getGrabber();
					auto & markerMap = markerMapNode->getMarkerMap();
					if (!markerMap->isExpressedInMeters()) {
						if (detectorNode) {
							markerMap->convertToMeters(detectorNode->getMarkerLength());
						}
					}

					//check if we have a new camera frame
					auto frame = cameraNode->getFrame();

					//do the tracking
					if (markerMap->empty()) {
						throw(ofxRulr::Exception("Marker map is empty"));
					}

					auto markers = detectorNode->findMarkers(ofxCv::toCv(frame->getPixels()));
					if (markers.size() < 3) {
						throw(ofxRulr::Exception("Couldn't find enough markers"));
					}

					auto cameraParams = aruco::CameraParameters();
					{
						cameraNode->getCameraMatrix().convertTo(cameraParams.CameraMatrix, CV_32F);
						cameraNode->getDistortionCoefficients().convertTo(cameraParams.Distorsion, CV_32F);
						cameraParams.CamSize = cameraNode->getSize();
					}

					this->markerMapPoseTracker.setParams(cameraParams
						, *markerMap
						, detectorNode->getMarkerLength());

					//try and estimate camera pose
					if (!this->markerMapPoseTracker.estimatePose(markers)) {
						throw(ofxRulr::Exception("Failed to estimate camera pose"));
					}

					cv::Mat rotation, translation;

					this->markerMapPoseTracker.getRvec().convertTo(rotation, CV_64F);
					this->markerMapPoseTracker.getTvec().convertTo(translation, CV_64F);

					cameraNode->setExtrinsics(rotation, translation, true);
				}
			}

			//----------
			void MarkerMapPoseTracker::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;
				inspector->addButton("Track", [this]() {
					try {
						this->track();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}
		}
	}
}
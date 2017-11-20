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

				this->addInput<Item::Camera>();
				this->addInput<MarkerMap>();
				this->addInput<Detector>();

				this->manageParameters(this->parameters);
			}
			aruco::MarkerMapPoseTracker markerMapPoseTracker;

			//----------
			void MarkerMapPoseTracker::update() {
				try {
					if (this->parameters.enabled) {
						auto cameraNode = this->getInput<Item::Camera>();
						auto detectorNode = this->getInput<Detector>();
						auto markerMapNode = this->getInput<MarkerMap>();

						if (cameraNode && detectorNode && markerMapNode) {
							auto grabber = cameraNode->getGrabber();
							if (grabber) {
								if (grabber->isFrameNew()) {
									auto & markerMap = markerMapNode->getMarkerMap();
									if (!markerMap->isExpressedInMeters()) {
											if (detectorNode) {
												markerMap->convertToMeters(detectorNode->getMarkerLength());
											}
										}

									//check if we have a new camera frame
									shared_ptr<ofxMachineVision::Frame> frame;
									{
										if (cameraNode) {
											if (cameraNode->getGrabber()->isFrameNew()) {
												frame = cameraNode->getFrame();
											}
										}
									}

									//do the tracking
									if(!markerMap->empty())
										try {
											auto & markerDetector = detectorNode->getMarkerDetector();
											auto markers = markerDetector.detect(ofxCv::toCv(frame->getPixels()));

											auto cameraParams = aruco::CameraParameters();
											{
												cameraNode->getCameraMatrix().convertTo(cameraParams.CameraMatrix, CV_32F);
												cameraNode->getDistortionCoefficients().convertTo(cameraParams.Distorsion, CV_32F);
												cameraParams.CamSize = cameraNode->getSize();
											}

											this->markerMapPoseTracker.setParams(cameraParams
												, * markerMap
												, detectorNode->getMarkerLength());

											//try and estimate camera pose
											if (this->markerMapPoseTracker.estimatePose(markers)) {
												cv::Mat rotation, translation;

												this->markerMapPoseTracker.getRvec().convertTo(rotation, CV_64F);
												this->markerMapPoseTracker.getTvec().convertTo(translation, CV_64F);

												//if successful then jump out
												cameraNode->setExtrinsics(rotation, translation, true);
											}
										}
										RULR_CATCH_ALL_TO_ERROR;
									}
								}
							}
						}
					}
				RULR_CATCH_ALL_TO_ERROR;
			}
		}
	}
}
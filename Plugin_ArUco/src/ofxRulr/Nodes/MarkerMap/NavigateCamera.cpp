#include "pch_Plugin_ArUco.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			//----------
			NavigateCamera::NavigateCamera() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string NavigateCamera::getTypeName() const {
				return "MarkerMap::NavigateCamera";
			}

			//----------
			void NavigateCamera::init() {
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::Camera>();
				this->addInput<Markers>();
			}

			//----------
			void NavigateCamera::update() {
				if(this->parameters.onNewFrame.get()) {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						if (camera->getGrabber()->isFrameNew()) {
							auto frame = camera->getGrabber()->getFrame();
							auto & pixels = frame->getPixels();
							if (pixels.isAllocated()) {
								auto image = ofxCv::toCv(pixels);
								this->track(image);
							}
						}
					}
				}
			}

			//----------
			void NavigateCamera::track(const cv::Mat& image) {
				this->throwIfMissingAnyConnection();

				auto markersNode = this->getInput<Markers>();
				auto camera = this->getInput<Item::Camera>();

				markersNode->throwIfMissingAConnection<ArUco::Detector>();
				auto detector = markersNode->getInput<ArUco::Detector>();

				auto markers = markersNode->getMarkers();
				if (markers.empty()) {
					throw(ofxRulr::Exception("No markers available"));
				}

				auto foundMarkers = detector->findMarkers(image);

				vector<cv::Point3f> worldPoints;
				vector<cv::Point2f> imagePoints;

				for (auto foundMarker : foundMarkers) {
					for (auto storedMarker : markers) {
						if (storedMarker->parameters.ID.get() == foundMarker.id) {
							const auto worldPointsToAdd = ofxCv::toCv(storedMarker->getWorldVertices());
							worldPoints.insert(worldPoints.end(), worldPointsToAdd.begin(), worldPointsToAdd.end());

							const auto& imagePointsToAdd = (vector<cv::Point2f>&) foundMarker;
							imagePoints.insert(imagePoints.end(), imagePointsToAdd.begin(), imagePointsToAdd.end());
						}
					}
				}

				if (worldPoints.empty()) {
					throw(ofxRulr::Exception("No matching markers found"));
				}

				cv::Mat rotationVector, translation;
				if (this->parameters.ransac.enabled.get()) {
					if (!cv::solvePnPRansac(worldPoints
						, imagePoints
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients()
						, rotationVector
						, translation
						, this->parameters.useExtrinsicGuess.get()
						, this->parameters.ransac.maxIterations.get()
						, this->parameters.ransac.reprojectionError.get())) {
						throw(ofxRulr::Exception("Failed to solvePnPRansac"));
					}
				}
				else {
					if (!cv::solvePnP(worldPoints
						, imagePoints
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients()
						, rotationVector
						, translation
						, this->parameters.useExtrinsicGuess.get()
						, cv::SOLVEPNP_ITERATIVE)) {
						throw(ofxRulr::Exception("Failed to solvePnP"));
					}
				}

				camera->setExtrinsics(rotationVector
					, translation
					, true);
			}

			//----------
			void NavigateCamera::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Track fresh frame", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Track fresh frame");
						this->throwIfMissingAnyConnection();
						auto camera = this->getInput<Item::Camera>();
						auto frame = camera->getFreshFrame();
						if (!frame) {
							throw(ofxRulr::Exception("Couldn't get fresh frame"));
						}
						if (frame->getPixels().getHeight() == 0) {
							throw(ofxRulr::Exception("Pixels empty"));
						}
						auto image = ofxCv::toCv(frame->getPixels());
						this->track(image);
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, ' ');

				inspector->addButton("Track prior frame", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Track prior frame");
						this->throwIfMissingAnyConnection();
						auto camera = this->getInput<Item::Camera>();
						auto frame = camera->getFrame();
						if (!frame) {
							throw(ofxRulr::Exception("No camera frame available"));
						}
						if (frame->getPixels().getHeight() == 0) {
							throw(ofxRulr::Exception("Pixels empty"));
						}
						auto image = ofxCv::toCv(frame->getPixels());
						this->track(image);
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}
		}
	}
}

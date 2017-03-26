#include "pch_Plugin_ArUco.h"
#include "MarkerMap.h"

#include "Detector.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			MarkerMap::MarkerMap() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			string MarkerMap::getTypeName() const {
				return "ArUco::MarkerMap";
			}

			//----------
			void MarkerMap::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Item::Camera>();
				this->addInput<Detector>();
			}

			//----------
			void MarkerMap::update() {
				auto cameraNode = this->getInput<Item::Camera>();
				auto detectorNode = this->getInput<Detector>();

				if (this->loadedMarkerMap != this->parameters.filename.get()) {
					if (this->parameters.filename.get().empty()) {
						this->markerMap.clear();
						this->loadedMarkerMap.clear();
					}
					else {
						try {
							this->markerMap.clear();
							this->markerMap.readFromFile(ofToDataPath(this->parameters.filename));
							this->loadedMarkerMap = this->parameters.filename;
						}
						RULR_CATCH_ALL_TO({
							RULR_ERROR << e.what();
							this->clear();
						});
					}
				}

				if (!this->markerMap.isExpressedInMeters() && !this->markerMap.empty()) {
					if (detectorNode) {
						this->markerMap.convertToMeters(detectorNode->getMarkerLength());
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
				if (this->parameters.updateCamera.get()
					&& (bool)detectorNode
					&& !this->markerMap.empty()
					&& (bool)frame) {
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
							, this->markerMap
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

			//----------
			void MarkerMap::drawWorld() {
				//get the detector for use previewing the markers
				auto detectorNode = this->getInput<Detector>();

				//build a mesh to use for previews 
				ofMesh markerPreview;
				{
					markerPreview.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);
					markerPreview.addTexCoords({
						{ 0, 0 }
						,{ ARUCO_PREVIEW_RESOLUTION, 0 }
						,{ ARUCO_PREVIEW_RESOLUTION, ARUCO_PREVIEW_RESOLUTION }
						,{ 0, ARUCO_PREVIEW_RESOLUTION }
					});
				}
				
				//cycle through markers and draw them
				for (const auto & marker3D : this->markerMap) {
					markerPreview.clearVertices();

					for (auto & vertex : marker3D) {
						markerPreview.addVertex(ofxCv::toOf(vertex));
					}

					if (detectorNode) {
						auto & image = detectorNode->getMarkerImage(marker3D.id);

						image.bind();
						{
							markerPreview.draw();
						}
						image.unbind();
					}
					else {
						markerPreview.draw();
					}

					ofDrawBitmapString(ofToString(marker3D.id), ofxCv::toOf(marker3D[0]));
				}
			}

			//----------
			void MarkerMap::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);

				inspector->addButton("Select file...", [this]() {
					try {
						auto result = ofSystemLoadDialog("Select marker map YML");
						if (result.bSuccess) {
							this->parameters.filename = result.filePath;
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
				inspector->addButton("Clear", [this]() {
					this->clear();
				});
			}

			//----------
			void MarkerMap::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void MarkerMap::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			void MarkerMap::clear() {
				this->loadedMarkerMap.clear();
				this->markerMap.clear();
				this->parameters.filename = "";
			}
		}
	}
}
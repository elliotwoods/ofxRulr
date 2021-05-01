#include "pch_Plugin_Calibrate.h"
#include "CameraExtrinsicsFromBoardPlanes.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Nodes/Item/Camera.h"

#include "ofxRulr/Utils/ScopedProcess.h"

#include "ofConstants.h"
#include "ofxCvGui.h"

using namespace ofxRulr::Graph;
using namespace ofxRulr::Nodes;

using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				CameraExtrinsicsFromBoardPlanes::CameraExtrinsicsFromBoardPlanes() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput(MAKE(Pin<Item::Camera>));
					this->addInput(MAKE(Pin<Item::AbstractBoard>));

					this->view = MAKE(ofxCvGui::Panels::Base);
					this->view->onDraw += [this](DrawArguments & drawArgs) {
						if (this->grayscalePreview.isAllocated()) {
							this->grayscalePreview.draw(drawArgs.localBounds);

							ofPushMatrix();
							{
								//scale view to camera coordinates
								auto cameraWidth = this->grayscalePreview.getWidth();
								auto cameraHeight = this->grayscalePreview.getHeight();
								ofScale(drawArgs.localBounds.getWidth() / cameraWidth, drawArgs.localBounds.getHeight() / cameraHeight);

								//draw current corners
								ofxCv::drawCorners(this->currentCorners);
							}
							ofPopMatrix();
						}

					};
				}

				//----------
				string CameraExtrinsicsFromBoardPlanes::getTypeName() const {
					return "Procedure::Calibrate::CameraExtrinsicsFromBoardPlanes";
				}

				//----------
				ofxCvGui::PanelPtr CameraExtrinsicsFromBoardPlanes::getPanel() {
					return this->view;
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::update() {

				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::drawWorldStage() {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						ofPushMatrix();
						{
							ofMultMatrix(camera->getTransform());

							ofPushStyle();
							{
								const glm::vec3 origin = this->parameters.calibrationPoints.originInCameraObjectSpace;
								const glm::vec3 positiveX = this->parameters.calibrationPoints.positiveXInCameraObjectSpace;
								const glm::vec3 pointInXZ = this->parameters.calibrationPoints.positionInXZPlaneInCameraObjectSpace;

								ofSetColor(255);
								ofFill();
								ofDrawSphere(origin, 0.08f);

								ofSetColor(255, 0, 0);
								ofDrawSphere(positiveX, 0.05f);

								ofSetColor(255, 0, 255);
								ofDrawSphere(pointInXZ, 0.05f);

								ofMesh lines;
								lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
								lines.addVertex(origin); lines.addColor(ofColor(255));
								lines.addVertex(positiveX); lines.addColor(ofColor(255, 0, 0));
								lines.addVertex(origin); lines.addColor(ofColor(255));
								lines.addVertex(pointInXZ); lines.addColor(ofColor(255, 0, 255));
								lines.draw();
							}
							ofPopStyle();
						}
						ofPopMatrix();
					}
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::serialize(nlohmann::json & json) {
					Utils::serialize(json, this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::deserialize(const nlohmann::json & json) {
					Utils::deserialize(json, this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;

					inspector->addButton("Capture origin", [this]() {
						try {
							this->captureOriginBoard();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, '1');
					inspector->addButton("Capture +X", [this]() {
						try {
							this->capturePositiveXBoard();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, '2');
					inspector->addButton("Capture point in XZ", [this]() {
						try {
							this->captureXZPlaneBoard();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, '3');
					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::calibrate() {
					this->throwIfMissingAConnection<Item::Camera>();
					auto camera = this->getInput<Item::Camera>();

					const glm::vec3 origin = this->parameters.calibrationPoints.originInCameraObjectSpace;
					const glm::vec3 positiveX = this->parameters.calibrationPoints.positiveXInCameraObjectSpace;
					const glm::vec3 pointInXZ = this->parameters.calibrationPoints.positionInXZPlaneInCameraObjectSpace;

					ofMatrix4x4 transform;
					
					{
						transform.translate(-origin);
					}

					{
						ofQuaternion rotate;
						rotate.makeRotate(positiveX - origin, glm::vec3(1, 0, 0));
						transform.rotate(rotate);
					}

					{
						auto pointInXZNow = Utils::applyTransform(transform, pointInXZ);
						pointInXZNow.x = 0.0f;
						ofQuaternion rotate;
						rotate.makeRotate(pointInXZNow
							, pointInXZNow.z < 0 ? glm::vec3(0, 0, -1) : glm::vec3(0, 0, +1));
						transform.rotate(rotate);
					}
					
					camera->setTransform((glm::mat4) transform);
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::captureOriginBoard() {
					this->captureTo(this->parameters.calibrationPoints.originInCameraObjectSpace);
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::capturePositiveXBoard() {
					this->captureTo(this->parameters.calibrationPoints.positiveXInCameraObjectSpace);

				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::captureXZPlaneBoard() {
					this->captureTo(this->parameters.calibrationPoints.positionInXZPlaneInCameraObjectSpace);
				}

				//----------
				void CameraExtrinsicsFromBoardPlanes::captureTo(ofParameter<glm::vec3> & calibrationPoint) {
					Utils::ScopedProcess scopedProcess("Capture board");

					this->throwIfMissingAnyConnection();

					auto camera = this->getInput<Item::Camera>();
					auto board = this->getInput<Item::AbstractBoard>();

					auto grabber = camera->getGrabber();
					auto frame = grabber->getFreshFrame();

					//capture the frame
					{
						if (!frame) {
							throw(Exception("No camera frame available"));
						}
						auto & pixels = frame->getPixels();
						if (!pixels.isAllocated()) {
							throw(Exception("Camera pixels are not allocated. Perhaps we need to wait for a frame?"));
						}
						if (this->grayscalePreview.getWidth() != pixels.getWidth() || this->grayscalePreview.getHeight() != pixels.getHeight()) {
							this->grayscalePreview.allocate(pixels.getWidth(), pixels.getHeight(), OF_IMAGE_GRAYSCALE);
						}
						if (pixels.getNumChannels() != 1) {
							cv::cvtColor(toCv(pixels), toCv(this->grayscalePreview), cv::COLOR_RGB2GRAY);
						}
						else {
							this->grayscalePreview = pixels;
						}

						this->grayscalePreview.update();
					}


					//find the corners
					{
						Utils::ScopedProcess scopedProcessFindBoard("Find board");

						this->currentCorners.clear();
						this->currentObjectPoints.clear();

						if (!board->findBoard(toCv(this->grayscalePreview)
							, toCv(this->currentCorners)
							, toCv(this->currentObjectPoints)
							, this->parameters.capture.findBoardMode.get()
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients())) {
							throw(Exception("Board not found in image"));
						}

						scopedProcessFindBoard.end();
					}
					

					//solve the transform
					glm::mat4 transform;
					{
						Mat rotation, translation;
						cv::solvePnP(ofxCv::toCv(this->currentObjectPoints)
							, ofxCv::toCv(this->currentCorners)
							, camera->getCameraMatrix()
							, camera->getDistortionCoefficients()
							, rotation, translation);

						transform = ofxCv::makeMatrix(rotation, translation);
					}

					//apply the result
					{
						glm::vec3 result = Utils::applyTransform(transform, glm::vec3());
						calibrationPoint = result;
					}

					scopedProcess.end();
				}

			}
		}
	}
}
#include "pch_RulrNodes.h"
#include "ProjectorFromDepthCamera.h"

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

using namespace ofxCvGui;
using namespace ofxCv;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				ProjectorFromDepthCamera::ProjectorFromDepthCamera() {
					RULR_NODE_INIT_LISTENER;
				}
				
				//----------
				string ProjectorFromDepthCamera::getTypeName() const {
					return "Procedure::Calibrate::ProjectorFromDepthCamera";
				}
				
				//----------
				PanelPtr ProjectorFromDepthCamera::getPanel() {
					return this->view;
				}

				//----------
				void ProjectorFromDepthCamera::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;
					
					auto depthCameraPin = this->addInput<Item::IDepthCamera>();
					this->addInput<Item::Projector>();
					auto videoOutputPin = this->addInput<System::VideoOutput>();
					
					this->view = make_shared<Panels::Draws>();
					this->view->onDrawCropped += [this](DrawCroppedArguments & args) {
						ofPolyline previewLine;
						if (!this->previewCornerFinds.empty()) {
							ofDrawCircle(this->previewCornerFinds.front(), 10.0f);
							for (const auto & previewCornerFind : this->previewCornerFinds) {
								previewLine.addVertex(previewCornerFind);
							}
						}
						ofPushStyle();
						ofSetColor(255, 0, 0);
						previewLine.draw();
						ofPopStyle();
					};
					
					this->checkerboard.scale.set("Checkerboard Scale", 0.2f, 0.01f, 1.0f);
					this->checkerboard.cornersX.set("Checkerboard Corners X", 5, 1, 10);
					this->checkerboard.cornersY.set("Checkerboard Corners Y", 4, 1, 10);
					this->checkerboard.positionX.set("Checkerboard Position X", 0, -1, 1);
					this->checkerboard.positionY.set("Checkerboard Position Y", 0, -1, 1);
					this->checkerboard.brightness.set("Checkerboard Brightness", 0.5, 0, 1);
					
					this->initialLensOffset.set("Initial Lens Offset", 0.5f, -1.0f, 1.0f);
				}
				
				//----------
				void ProjectorFromDepthCamera::update() {
					//--
					//set the draw object each frame incase anything changed
					//--
					//
					auto depthCamera = this->getInput<Item::IDepthCamera>();
					if(depthCamera) {
						auto colorInDepth = depthCamera->getColorInDepthImage();
						if(colorInDepth) {
							this->view->setDrawObject(*colorInDepth);
						}
					} else {
						this->view->clearDrawObject();
					}
					//
					//--
				
				
					
					//--
					//draw on video output
					//--
					//
					auto videoOutput = this->getInput<System::VideoOutput>();
					if (videoOutput) {
						if (videoOutput->isWindowOpen()) {
							videoOutput->getFbo().begin();
							ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
							ofLoadIdentityMatrix();
							ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
							ofLoadIdentityMatrix();
							
							ofPushStyle();
							ofSetColor(255.0f * this->checkerboard.brightness);
							ofDrawRectangle(-1, -1, 2, 2);
							ofPopStyle();
							
							ofTranslate(this->checkerboard.positionX, this->checkerboard.positionY);
							auto checkerboardMesh = ofxCv::makeCheckerboardMesh(cv::Size(this->checkerboard.cornersX, this->checkerboard.cornersY), this->checkerboard.scale);
							auto & colors = checkerboardMesh.getColors();
							for (auto & color : colors) {
								color *= this->checkerboard.brightness;
							}
							checkerboardMesh.draw();
							
							videoOutput->getFbo().end();
						}
					}
					//
					//--
				}
				
				//----------
				void ProjectorFromDepthCamera::serialize(Json::Value & json) {
					auto & jsonCheckerboard = json["checkerboard"];
					{
						ofxRulr::Utils::Serializable::serialize(this->checkerboard.scale, jsonCheckerboard);
						ofxRulr::Utils::Serializable::serialize(this->checkerboard.cornersX, jsonCheckerboard);
						ofxRulr::Utils::Serializable::serialize(this->checkerboard.cornersY, jsonCheckerboard);
						ofxRulr::Utils::Serializable::serialize(this->checkerboard.positionX, jsonCheckerboard);
						ofxRulr::Utils::Serializable::serialize(this->checkerboard.positionY, jsonCheckerboard);
						ofxRulr::Utils::Serializable::serialize(this->checkerboard.brightness, jsonCheckerboard);
					}
					ofxRulr::Utils::Serializable::serialize(this->initialLensOffset, json);
					
					auto & jsonCorrespondences = json["correspondences"];
					int index = 0;
					for (const auto & correspondence : this->correspondences) {
						auto & jsonCorrespondence = jsonCorrespondences[index++];
						for (int i = 0; i < 3; i++) {
							jsonCorrespondence["world"][i] = correspondence.world[i];
						}
						for (int i = 0; i < 2; i++) {
							jsonCorrespondence["projector"][i] = correspondence.projector[i];
						}
					}
					
					json["error"] = this->error;
				}
				
				//----------
				void ProjectorFromDepthCamera::deserialize(const Json::Value & json) {
					const auto & jsonCheckerboard = json["checkerboard"];
					{
						ofxRulr::Utils::Serializable::deserialize(this->checkerboard.scale, jsonCheckerboard);
						ofxRulr::Utils::Serializable::deserialize(this->checkerboard.cornersX, jsonCheckerboard);
						ofxRulr::Utils::Serializable::deserialize(this->checkerboard.cornersY, jsonCheckerboard);
						ofxRulr::Utils::Serializable::deserialize(this->checkerboard.positionX, jsonCheckerboard);
						ofxRulr::Utils::Serializable::deserialize(this->checkerboard.positionY, jsonCheckerboard);
						ofxRulr::Utils::Serializable::deserialize(this->checkerboard.brightness, jsonCheckerboard);
					}
					ofxRulr::Utils::Serializable::deserialize(this->initialLensOffset, json);
					
					this->correspondences.clear();
					auto & jsonCorrespondences = json["correspondences"];
					for (const auto & jsonCorrespondence : jsonCorrespondences) {
						Correspondence correspondence;
						for (int i = 0; i < 3; i++) {
							correspondence.world[i] = jsonCorrespondence["world"][i].asFloat();
						}
						for (int i = 0; i < 2; i++) {
							correspondence.projector[i] = jsonCorrespondence["projector"][i].asFloat();
						}
						this->correspondences.push_back(correspondence);
					}
					
					this->error = json["error"].asFloat();
				}
				
				//----------
				void ProjectorFromDepthCamera::addCapture() {
					this->throwIfMissingAnyConnection();
					
					auto depthCameraNode = this->getInput<Item::IDepthCamera>();
					auto colorInDepth = depthCameraNode->getColorInDepthImage();
					auto world = depthCameraNode->getWorldPixels();
					
					if(!colorInDepth) {
						throw(Exception("Cannot getColorInDepthImage."));
					}
					if(!world) {
						throw(Exception("Cannot getWorldPixels"));
					}
					
					//flip the color image
					auto colorImage = toCv(colorInDepth->getPixels());
					flip(colorImage, colorImage, 1);
					
					//find the checkerboard
					vector<ofVec2f> depthMapPoints;
					if (!ofxCv::findChessboardCornersPreTest(toCv(colorInDepth->getPixels()), cv::Size(this->checkerboard.cornersX, this->checkerboard.cornersY), toCv(depthMapPoints))) {
						throw(ofxRulr::Exception("Couldn't find checkerboard in mapped color image"));
					};
					
					//flip the results back again
					const auto depthMapWidth = colorInDepth->getWidth();
					for (auto & depthMapPoint : depthMapPoints) {
						depthMapPoint.x = depthMapWidth - depthMapPoint.x - 1;
					}
					
					this->previewCornerFinds.clear();
					
					auto checkerboardCorners = toOf(ofxCv::makeCheckerboardPoints(cv::Size(this->checkerboard.cornersX, this->checkerboard.cornersY), this->checkerboard.scale, true));
					int pointIndex = 0;

					const auto worldPoints = (ofVec3f*) world->getData();
					for (auto depthMapPoint : depthMapPoints) {
						this->previewCornerFinds.push_back(depthMapPoint);
						Correspondence correspondence;
						
						correspondence.world = worldPoints[(int)depthMapPoint.x + (int)depthMapPoint.y * (int)depthMapWidth];
						correspondence.projector = (ofVec2f)checkerboardCorners[pointIndex] + ofVec2f(this->checkerboard.positionX, this->checkerboard.positionY);
						
						//check correspondence has valid z coordinate before adding it to the calibration set
						if (correspondence.world.z > 0.2f) {
							this->correspondences.push_back(correspondence);
						}
						
						pointIndex++;
					}
				}
				
				//----------
				void ProjectorFromDepthCamera::calibrate() {
					this->throwIfMissingAnyConnection();
					
					auto projector = this->getInput<Item::Projector>();
					auto videoOutput = this->getInput<System::VideoOutput>();
					auto depthCamera = this->getInput<Item::IDepthCamera>();
					
					auto depthCameraTransform = depthCamera->getTransform();
					
					//update projector width and height to match the video output
					projector->setWidth(videoOutput->getWidth());
					projector->setHeight(videoOutput->getHeight());
					
					vector<ofVec3f> worldPoints;
					vector<ofVec2f> projectorPoints;
					
					for (auto correpondence : this->correspondences) {
						worldPoints.push_back(correpondence.world * depthCameraTransform);
						projectorPoints.push_back(correpondence.projector);
					}
					cv::Mat cameraMatrix, rotation, translation;
					this->error = ofxCv::calibrateProjector(cameraMatrix, rotation, translation,
															worldPoints, projectorPoints,
															this->getInput<Item::Projector>()->getWidth(), this->getInput<Item::Projector>()->getHeight(),
															this->initialLensOffset, 1.4f);
					
					auto view = ofxCv::makeMatrix(rotation, translation);
					projector->setTransform(view.getInverse());
					projector->setIntrinsics(cameraMatrix);
				}
				
				//----------
				void ProjectorFromDepthCamera::drawWorld() {
					ofMesh preview;
					for (auto correspondence : this->correspondences) {
						preview.addVertex(correspondence.world);
						preview.addColor(ofColor(
												 ofMap(correspondence.projector.x, -1, 1, 0, 255),
												 ofMap(correspondence.projector.y, -1, 1, 0, 255),
												 0));
					}
					glPushAttrib(GL_POINT_BIT);
					glEnable(GL_POINT_SMOOTH);
					glPointSize(10.0f);
					preview.drawVertices();
					glPopAttrib();
				}
				
				//----------
				void ProjectorFromDepthCamera::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					auto slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboard.scale);
					inspector->add(slider);
					
					slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboard.cornersX);
					slider->addIntValidator();
					inspector->add(slider);
					
					slider = MAKE(ofxCvGui::Widgets::Slider, this->checkerboard.cornersY);
					slider->addIntValidator();
					inspector->add(slider);
					
					inspector->add(new ofxCvGui::Widgets::LiveValue<string>("Warning", [this]() {
						bool xOdd = (int) this->checkerboard.cornersX & 1;
						bool yOdd = (int) this->checkerboard.cornersY & 1;
						if (xOdd && yOdd) {
							return "Corners X and Corners Y both odd";
						}
						else if (!xOdd && !yOdd) {
							return "Corners X and Corners Y both even";
						}
						else {
							return "";
						}
					}));
					
					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->checkerboard.positionX));
					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->checkerboard.positionY));
					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->checkerboard.brightness));
					
					auto addButton = MAKE(ofxCvGui::Widgets::Button, "Add Capture", [this]() {
						try {
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ERROR
					}, ' ');
					addButton->setHeight(100.0f);
					inspector->add(addButton);
					
					inspector->add(MAKE(ofxCvGui::Widgets::Button, "Clear correspondences", [this]() {
						this->correspondences.clear();
					}));
					
					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->initialLensOffset));

					auto calibrateButton = MAKE(ofxCvGui::Widgets::Button, "Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);
					inspector->add(MAKE(ofxCvGui::Widgets::LiveValue<float>, "Reprojection error [px]", [this]() {
						return this->error;
					}));
				}
			}
		}
	}
}
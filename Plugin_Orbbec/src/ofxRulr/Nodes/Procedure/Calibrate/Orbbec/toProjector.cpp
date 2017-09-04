#include "pch_Plugin_Orbbec.h"
#include "toPojector.h"

#include "ofxRulr/Nodes/Item/Orbbec/Device.h"
#include "ofxRulr/Nodes/Item/Orbbec/Color.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace Orbbec {
					//----------
					ToProjector::ToProjector() {
						RULR_NODE_INIT_LISTENER;
					}

					//----------
					string ToProjector::getTypeName() const {
						return "Procedure::Calibrate::Orbbec::ToProjector";
					}

					//----------
					void ToProjector::init() {
						RULR_NODE_INSPECTOR_LISTENER;
						RULR_NODE_SERIALIZATION_LISTENERS;
						RULR_NODE_DRAW_WORLD_LISTENER;

						this->addInput<Item::Orbbec::Device>();
						this->addInput<Item::Orbbec::Color>();
						this->addInput<Item::Projector>();
						auto videoOutputPin = this->addInput<System::VideoOutput>();

						this->manageParameters(this->parameters);

						videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
							videoOutput->onDrawOutput.addListener([this](ofRectangle & output) {
								this->drawVideoOutput();
							}, this);
						};
						videoOutputPin->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
							if (videoOutput) {
								videoOutput->onDrawOutput.removeListeners(this);
							}
						};
					}

					//----------
					void ToProjector::populateInspector(ofxCvGui::InspectArguments & args) {
						auto inspector = args.inspector;
						{
							auto button = make_shared<ofxCvGui::Widgets::Button>("Add capture", [this]() {
								try {
									Utils::ScopedProcess scopedProcess("Add capture");
									this->addCapture();
									scopedProcess.end();
								}
								RULR_CATCH_ALL_TO_ERROR;
							}, ' ');
							inspector->add(button);
						}
						inspector->addLiveValue<size_t>("Correspondence count", [this]() {
							return this->correspondences.size();
						});
						inspector->addButton("Clear all ", [this]() {
							this->correspondences.clear();
						});
						inspector->addButton("Clear last", [this]() {
							if (!this->correspondences.empty()) {
								this->correspondences.resize(this->correspondences.size() - 1);
							}
						});
						{
							auto button = make_shared<ofxCvGui::Widgets::Button>("Calibrate", [this]() {
								try {
									Utils::ScopedProcess scopedProcess("Calibrate");
									this->calibrate();
									scopedProcess.end();
								}
								RULR_CATCH_ALL_TO_ERROR;
							}, OF_KEY_RETURN);
							button->setHeight(100.0f);
							inspector->add(button);
						}
						inspector->addLiveValue<float>("Reprojection error [px]", [this]() {
							return this->error;
						});
					}

					//----------
					void ToProjector::drawWorld() {
						ofMesh preview;
						for (auto correspondence : this->correspondences) {
							preview.addVertex(correspondence.world);
							preview.addColor(ofColor(
								ofMap(correspondence.projector.x, -1, 1, 0, 255),
								ofMap(correspondence.projector.y, -1, 1, 0, 255),
								0));
						}

						preview.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
						preview.drawVertices();
					}

					//----------
					void ToProjector::serialize(Json::Value & json) {
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

						json["error"] = this->error.get();
					}

					//----------
					void ToProjector::deserialize(const Json::Value & json) {
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
					void ToProjector::addCapture() {
						this->throwIfMissingAnyConnection();

						auto deviceNode = this->getInput<Item::Orbbec::Device>();
						auto device = deviceNode->getDevice();
						if (!device) {
							throw(ofxRulr::Exception("No device initialised."));
						}
						auto colorStream = device->getColor();
						if (!colorStream) {
							throw(ofxRulr::Exception("Color stream not initialised"));
						}

						auto colorNode = this->getInput<Item::Orbbec::Color>();

						vector<ofVec2f> foundCorners;
						cv::findChessboardCorners(toCv(colorStream->getPixels()), cv::Size(this->parameters.checkerboard.cornersX, this->parameters.checkerboard.cornersY), toCv(foundCorners));

						if (foundCorners.empty()) {
							throw(ofxRulr::Exception("No corners found."));
						}

						auto checkerboardCorners = toOf(ofxCv::makeCheckerboardPoints(cv::Size(this->parameters.checkerboard.cornersX, this->parameters.checkerboard.cornersY), this->parameters.checkerboard.scale, true));

						vector<Correspondence> newCorrespondences;
						int pointIndex = 0;
						for (const auto & foundCorner : foundCorners) {
							auto foundCornerWorld = colorNode->cameraToWorld(foundCorner);
							if (foundCornerWorld.z == 0.0f) {
								continue;
							}

							Correspondence correspondence{
								foundCornerWorld,
								(ofVec2f)checkerboardCorners[pointIndex] + ofVec2f(this->parameters.checkerboard.positionX, this->parameters.checkerboard.positionY)
							};

							newCorrespondences.push_back(correspondence);
							pointIndex++;
						}
						
						if (newCorrespondences.empty()) {
							throw(ofxRulr::Exception("Can't find world locations for corners."));
						}

						this->correspondences.insert(this->correspondences.end(), newCorrespondences.begin(), newCorrespondences.end());
					}

					//----------
					void ToProjector::calibrate() {
						this->throwIfMissingAnyConnection();

						auto projector = this->getInput<Item::Projector>();
						auto videoOutput = this->getInput<System::VideoOutput>();

						//update projector width and height to match the video output
						projector->setWidth(videoOutput->getWidth());
						projector->setHeight(videoOutput->getHeight());

						vector<ofVec3f> worldPoints;
						vector<ofVec2f> projectorPoints;

						for (auto correpondence : this->correspondences) {
							worldPoints.push_back(correpondence.world);
							projectorPoints.push_back(correpondence.projector);
						}
						cv::Mat cameraMatrix, rotation, translation;
						this->error = ofxCv::calibrateProjector(cameraMatrix, rotation, translation,
							worldPoints, projectorPoints,
							this->getInput<Item::Projector>()->getWidth(), this->getInput<Item::Projector>()->getHeight(),
							true,
							this->parameters.calibration.initialLensOffset, this->parameters.calibration.initialThrowRatio);

						auto view = ofxCv::makeMatrix(rotation, translation);
						projector->setTransform(view.getInverse());
						projector->setIntrinsics(cameraMatrix);
					}

					//----------
					void ToProjector::drawVideoOutput() {
						ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
						ofLoadIdentityMatrix();
						ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
						ofLoadIdentityMatrix();

						ofPushStyle();
						ofSetColor(255.0f * this->parameters.checkerboard.brightness);
						ofDrawRectangle(-1, -1, 2, 2);
						ofPopStyle();

						ofTranslate(this->parameters.checkerboard.positionX, this->parameters.checkerboard.positionY);
						auto checkerboardMesh = ofxCv::makeCheckerboardMesh(cv::Size(this->parameters.checkerboard.cornersX, this->parameters.checkerboard.cornersY), this->parameters.checkerboard.scale);
						auto & colors = checkerboardMesh.getColors();
						for (auto & color : colors) {	
							color *= this->parameters.checkerboard.brightness;
						}
						checkerboardMesh.draw();
					}
				}
			}
		}
	}
}
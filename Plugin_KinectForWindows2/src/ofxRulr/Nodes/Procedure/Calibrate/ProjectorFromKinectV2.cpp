#include "pch_Plugin_KinectForWindows2.h"
#include "ProjectorFromKinectV2.h"

#include "ofxRulr/Nodes/Item/KinectV2.h"

#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"

using namespace ofxRulr::Graph;
using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				ProjectorFromKinectV2::ProjectorFromKinectV2() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string ProjectorFromKinectV2::getTypeName() const {
					return "Procedure::Calibrate::ProjectorFromKinectV2";
				}

				//----------
				void ProjectorFromKinectV2::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					auto kinectPin = MAKE(Pin<Item::KinectV2>);
					this->addInput(kinectPin);
					this->addInput(MAKE(Pin<Item::Projector>));
					this->addInput(MAKE(Pin<System::VideoOutput>));

					this->view = MAKE(ofxCvGui::Panels::Draws);
					this->view->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						auto captures = this->captures.getSelection();
						ofPushStyle();
						{
							for (auto capture : captures) {
								ofSetColor(capture->color);
								ofxCv::drawCorners(capture->cameraImagePoints, true);
							}
						}
						ofPopStyle();
					};

					kinectPin->onNewConnection += [this](shared_ptr<Item::KinectV2> & kinectNode) {
						if (kinectNode) {
							auto source = kinectNode->getDevice()->getColorSource();
							this->view->setDrawObject(*source);
						}
					};
					kinectPin->onDeleteConnectionUntyped += [this](shared_ptr<Nodes::Base> &) {
						this->view->clearDrawObject();
					};

					this->error = 0.0f;
				}

				//----------
				ofxCvGui::PanelPtr ProjectorFromKinectV2::getPanel() {
					return view;
				}

				//----------
				void ProjectorFromKinectV2::update() {
					auto projectorOutput = this->getInput<System::VideoOutput>();
					if (projectorOutput) {
						if (projectorOutput->isWindowOpen()) {
							projectorOutput->getFbo().begin();
							ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
							ofLoadIdentityMatrix();
							ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
							ofLoadIdentityMatrix();

							ofPushStyle();
							ofSetColor(255.0f * this->parameters.checkerboardBrightness);
							ofDrawRectangle(-1, -1, 2, 2);
							ofPopStyle();

							ofTranslate(this->parameters.checkerboardPositionX, this->parameters.checkerboardPositionY);
							auto checkerboardMesh = ofxCv::makeCheckerboardMesh(cv::Size(this->parameters.checkerboardCornersX, this->parameters.checkerboardCornersY), this->parameters.checkerboardScale);
							auto & colors = checkerboardMesh.getColors();
							for (auto & color : colors) {
								color *= this->parameters.checkerboardBrightness;
							}
							checkerboardMesh.draw();

							projectorOutput->getFbo().end();
						}
					}
				}

				//----------
				void ProjectorFromKinectV2::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);

					this->captures.serialize(json);
					json["error"] = this->error;
				}

				//----------
				void ProjectorFromKinectV2::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);

					this->captures.deserialize(json);
					this->error = json["error"].asFloat();
				}

				//----------
				void ProjectorFromKinectV2::addCapture() {
					this->throwIfMissingAnyConnection();

					auto kinectNode = this->getInput<Item::KinectV2>();
					auto kinectDevice = kinectNode->getDevice();
					auto colorPixels = kinectDevice->getColorSource()->getPixels();
					auto colorImage = ofxCv::toCv(colorPixels);

					//flip the camera image
					cv::flip(colorImage, colorImage, 1);

					vector<ofVec2f> cameraPoints;
					{
						cv::Mat colorAsGrayscale;
						cv::cvtColor(colorImage, colorAsGrayscale, CV_RGBA2GRAY);
						if (!ofxCv::findChessboardCornersPreTest(colorAsGrayscale, cv::Size(this->parameters.checkerboardCornersX, this->parameters.checkerboardCornersY), toCv(cameraPoints))) {
							throw(ofxRulr::Exception("Checkerboard not found in color image"));
						}
					}

					//flip the results back again
					int colorWidth = colorPixels.getWidth();
					for (auto & cameraPoint : cameraPoints) {
						cameraPoint.x = colorWidth - cameraPoint.x - 1;
					}

					ofFloatPixels cameraToWorldMap;
					kinectDevice->getDepthSource()->getWorldInColorFrame(cameraToWorldMap);
					auto cameraToWorldPointer = (ofVec3f*)cameraToWorldMap.getData();
					auto cameraWidth = cameraToWorldMap.getWidth();
					auto checkerboardCorners = toOf(ofxCv::makeCheckerboardPoints(cv::Size(this->parameters.checkerboardCornersX, this->parameters.checkerboardCornersY), this->parameters.checkerboardScale, true));
					int pointIndex = 0;

					auto capture = make_shared<Capture>();
					for (auto cameraPoint : cameraPoints) {
						auto worldSpace = cameraToWorldPointer[(int)cameraPoint.x + (int)cameraPoint.y * cameraWidth];

						if (worldSpace.z < 0.1f) {
							continue;
						}

						capture->cameraImagePoints.push_back(cameraPoint);
						capture->worldSpace.push_back(worldSpace);
						capture->projectorImageSpace.push_back((ofVec2f)checkerboardCorners[pointIndex] + ofVec2f(this->parameters.checkerboardPositionX, this->parameters.checkerboardPositionY));
						pointIndex++;
					}
					this->captures.add(capture);
				}

				//----------
				void ProjectorFromKinectV2::calibrate() {
					this->throwIfMissingAnyConnection();

					auto projector = this->getInput<Item::Projector>();
					auto videoOutput = this->getInput<System::VideoOutput>();

					//update projector width and height to match the video output
					projector->setWidth(videoOutput->getWidth());
					projector->setHeight(videoOutput->getHeight());

					vector<ofVec3f> worldPoints;
					vector<ofVec2f> projectorPoints;

					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						for (const auto & worldPoint : capture->worldSpace) {
							worldPoints.push_back(worldPoint);
						}
						for (const auto & projectorPoint : capture->projectorImageSpace) {
							projectorPoints.push_back(projectorPoint);
						}
					}
					cv::Mat cameraMatrix, rotation, translation;
					this->error = ofxCv::calibrateProjector(cameraMatrix, rotation, translation,
						worldPoints, projectorPoints,
						this->getInput<Item::Projector>()->getWidth(), this->getInput<Item::Projector>()->getHeight(),
						true,
						this->parameters.initialLensOffset, 1.4f, this->parameters.trimOutliers);

					auto view = ofxCv::makeMatrix(rotation, translation);
					projector->setTransform(view.getInverse());
					projector->setIntrinsics(cameraMatrix);
				}

				//----------
				void ProjectorFromKinectV2::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					auto slider = MAKE(ofxCvGui::Widgets::Slider, this->parameters.checkerboardScale);
					inspector->add(slider);

					slider = MAKE(ofxCvGui::Widgets::Slider, this->parameters.checkerboardCornersX);
					slider->addIntValidator();
					inspector->add(slider);

					slider = MAKE(ofxCvGui::Widgets::Slider, this->parameters.checkerboardCornersY);
					slider->addIntValidator();
					inspector->add(slider);

					inspector->add(new ofxCvGui::Widgets::LiveValue<string>("Warning", [this]() {
						bool xOdd = (int) this->parameters.checkerboardCornersX & 1;
						bool yOdd = (int) this->parameters.checkerboardCornersY & 1;
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

					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->parameters.checkerboardPositionX));
					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->parameters.checkerboardPositionY));
					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->parameters.checkerboardBrightness));

					auto addButton = MAKE(ofxCvGui::Widgets::Button, "Add Capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("ProjectorFromKinectV2 - addCapture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR
					}, ' ');

					addButton->setHeight(100.0f);
					inspector->add(addButton);
					this->captures.populateWidgets(inspector);

					inspector->add(MAKE(ofxCvGui::Widgets::Slider, this->parameters.initialLensOffset));
					inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->parameters.trimOutliers));
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

				//----------
				void ProjectorFromKinectV2::drawWorldStage() {
					auto kinect = this->getInput<Item::KinectV2>();
					auto projector = this->getInput<Item::Projector>();

					auto captures = this->captures.getSelection();
					ofPushStyle();
					{
						for (auto capture : captures) {
							ofSetColor(capture->color);
							ofxCv::drawCorners(capture->worldSpace);
						}
					}
					ofPopStyle();
				}

				//----------
				ProjectorFromKinectV2::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				void ProjectorFromKinectV2::Capture::serialize(Json::Value & json) {
					json["worldSpace"] >> this->worldSpace;
					json["projectorImageSpace"] >> this->projectorImageSpace;
				}

				//----------
				void ProjectorFromKinectV2::Capture::deserialize(const Json::Value & json) {
					json["worldSpace"] >> this->worldSpace;
					json["projectorImageSpace"] >> this->projectorImageSpace;
				}

				//----------
				std::string ProjectorFromKinectV2::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->worldSpace.size() << " points found.";
					return ss.str();
				}

			}
		}
	}
}
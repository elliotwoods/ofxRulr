#include "pch_RulrNodes.h"
#include "ARCube.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

#include "ofxCvMin.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/MultipleChoice.h"
#include "ofxCvGui/Widgets/Slider.h"

using namespace ofxCvGui;
using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			//----------
			ARCube::ARCube() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string ARCube::getTypeName() const {
				return "Test::ARCube";
			}

			//----------
			void ARCube::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<Item::Camera>();
				this->addInput<Item::AbstractBoard>();

				this->view = make_shared<Panels::Draws>(this->fbo);
				this->view->onDraw += [this](DrawArguments & args) {
					if (!this->getRunFinderEnabled()) {
						ofxCvGui::Utils::drawText("Select this node and connect active camera.", args.localBounds);
					}
				};

				//remove complaints from log about not allocated
				this->fbo.allocate(1, 1, OF_IMAGE_COLOR_ALPHA);

				//--
				//Setup box
				//--
				//
				//colour vertices
				{
					//set dimensions to 1.0f and move it forwards so sitting on z=0
					this->cube.set(1.0f);
					this->cube.move(ofVec3f(0, 0, -0.5f));

					//setup the colours
					auto & cubeMesh = this->cube.getMesh();
					auto & colours = cubeMesh.getColors();
					const auto & vertices = cubeMesh.getVertices();
					auto numVertices = vertices.size();
					colours.resize(numVertices);
					for (int i = 0; i < numVertices; i++) {
						for (int j = 0; j < 3; j++) {
							colours[i][j] = ofMap(vertices[i][j], -0.5f, 0.5f, 0.3f, 1.0f);
						}
						colours[i].a = 1.0f;
					}
				}
				//
				//--

				this->foundBoard = false;
			}

			//----------
			void ARCube::update() {
				auto camera = this->getInput<Item::Camera>();
				if (camera) {
					auto grabber = camera->getGrabber();
					if (grabber->isFrameNew()) {
						//if we're not using freerun, then we presume we're using single shot (since there's a frame available)
						if (this->getRunFinderEnabled()) {
							//allocate the undistorted image and fbo when required
							auto distorted = grabber->getPixels();
							auto & undistorted = this->undistorted.getPixels();
							if (undistorted.getWidth() != distorted.getWidth() || undistorted.getHeight() != distorted.getHeight() || undistorted.getNumChannels() != distorted.getNumChannels()) {
								//if undistorted isn't the right shape/format, then reallocate undistorted and the fbo
								undistorted = distorted;

								ofFbo::Settings fboSettings;
								fboSettings.width = undistorted.getWidth();
								fboSettings.height = undistorted.getHeight();
								fboSettings.useDepth = true;
								fboSettings.depthStencilInternalFormat = GL_DEPTH_COMPONENT24;
								fboSettings.minFilter = GL_NEAREST;
								fboSettings.maxFilter = GL_NEAREST;
								this->fbo.allocate(fboSettings);
							}

							//perform computer vision tasks
							try {
								auto cameraMatrix = camera->getCameraMatrix();
								auto distortionCoefficients = camera->getDistortionCoefficients();

								//make the undistorted preview image
								cv::undistort(toCv(distorted), toCv(undistorted), cameraMatrix, distortionCoefficients);
								this->undistorted.update();

								//find the board
								auto board = this->getInput<Item::AbstractBoard>();
								if (board) {
									//find board in distorted image
									vector<cv::Point2f> imagePoints;
									vector<cv::Point3f> objectPoints;
									auto success = board->findBoard(toCv(distorted)
										, imagePoints
										, objectPoints
										, this->parameters.findBoardMode
										, camera->getCameraMatrix()
										, camera->getDistortionCoefficients());
									if (success) {
										//use solvePnP to resolve transform
										Mat rotation, translation;
										cv::solvePnP(objectPoints
											, imagePoints
											, cameraMatrix
											, distortionCoefficients
											, rotation
											, translation);

										this->boardTransform = makeMatrix(rotation, translation);
										this->foundBoard = true;
									}
									else {
										this->foundBoard = false;
									}
								}
							}
							RULR_CATCH_ALL_TO_ERROR

							//update the fbo
							this->fbo.begin();
							{
								//draw the undistorted image
								this->undistorted.draw(0, 0);

								//draw the 3D world on top
								const auto & view = camera->getViewInWorldSpace();
								view.beginAsCamera(true);
								glEnable(GL_DEPTH_TEST);
								glClear(GL_DEPTH_BUFFER_BIT);
								this->drawWorldStage();
								glDisable(GL_DEPTH_TEST);
								view.endAsCamera();
							}
							this->fbo.end();
						}
					}
				}
			}

			//----------
			void ARCube::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				inspector->addParameterGroup(this->parameters);

				inspector->add(new Widgets::Indicator("Found board", [this]() {
					if (this->getInputPin<Item::AbstractBoard>()->isConnected() && this->getInputPin<Item::Camera>()->isConnected()) {
						if (this->foundBoard) {
							return Widgets::Indicator::Status::Good;
						}
						else {
							return Widgets::Indicator::Status::Clear;
						}
					}
					else {
						return Widgets::Indicator::Status::Error;
					}
				}));
				inspector->add(new Widgets::LiveValue<string>("Board position", [this]() {
					if (this->foundBoard) {
						stringstream ss;
						ss << (ofVec3f() * this->boardTransform);
						return ss.str();
					}
					else {
						return string("");
					}
				}));
			}

			//----------
			void ARCube::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void ARCube::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			void ARCube::drawWorldStage() {
				auto camera = this->getInput<Item::Camera>();
				auto board = this->getInput<Item::AbstractBoard>();
				if (board && camera && this->foundBoard) {
					ofPushStyle();
					{
						ofPushMatrix();
						{
							auto spacing = board->getSpacing();

							ofMultMatrix(camera->getTransform());
							ofMultMatrix(this->boardTransform);

							switch (this->parameters.drawStyle.get()) {
							case DrawStyle::Axes:
							{
								ofDrawAxis(spacing);
								auto position = ofVec3f() * this->boardTransform;
								ofPushStyle();
								{
									ofDrawBitmapString(ofToString(position), ofVec3f());
								}
								ofPopStyle();
								break;
							}
							case DrawStyle::Cube:
							{
								ofPushMatrix();
								{
									auto scale = board->getSpacing() * 4.0f;
									ofScale(scale, scale, scale);
									ofPushStyle();
									{
										if (this->parameters.fillMode.get() == FillMode::Fill) {
											ofFill();
										}
										else {
											ofNoFill();
										}
										this->cube.draw();
									}
									ofPopStyle();
								}
								ofPopMatrix();
								break;
							}
							case DrawStyle::Board:
							{
								if (this->parameters.fillMode.get() == FillMode::Fill) {
									ofFill();
								}
								else {
									ofNoFill();
								}
								board->drawObject();
								break;
							}
							default:
								break;
							}
						}
						ofPopMatrix();
					}
					ofPopStyle();
				}
			}

			//----------
			bool ARCube::getRunFinderEnabled() const {
				auto camera = this->getInput<Item::Camera>();
				if(!camera) {
					return false;
				}
				auto grabber = camera->getGrabber();
				if(!grabber->getIsDeviceOpen()) {
					return false;
				}
				
				switch (this->parameters.activewhen.get()) {
				case ActiveWhen::Always:
					return true;
				case ActiveWhen::Selected:
					if (this->isBeingInspected()) {
						return true;
					}
					break;
				default:
					break;
				}

				if (camera) {
					auto grabber = camera->getGrabber();
					if (!grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::Continuous)) {
						//find when the camera is a single shot camera
						return true;
					}
				}
				
				//if nothing was good, then return false
				return false;
			}

			//----------
			ofxCvGui::PanelPtr ARCube::getPanel() {
				return this->view;
			}
		}
	}
}
#include "ARCube.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Board.h"

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
		namespace Demo {
			//----------
			ARCube::ARCube() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string ARCube::getTypeName() const {
				return "Demo::ARCube";
			}

			//----------
			void ARCube::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<Item::Camera>();
				this->addInput<Item::Board>();

				this->view = make_shared<Panels::Draws>(this->fbo);
				this->view->onDraw += [this](DrawArguments & args) {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						if (!this->getRunFinderEnabled()) {
							ofxCvGui::Utils::drawText("Select this node to enable demo...", args.localBounds);
						}
					}
					else {
						ofxCvGui::Utils::drawText("Connect Camera and Board to enable demo...", args.localBounds);
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

				this->activewhen.set("Active when", 0, 0, 1);
				this->drawStyle.set("Draw style", 2, 0, 2);
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
								auto board = this->getInput<Item::Board>();
								if (board) {
									//find board in distorted image
									vector<ofVec2f> imagePoints;
									auto success = board->findBoard(toCv(distorted), toCv(imagePoints));
									if (success) {
										//use solvePnP to resolve transform
										Mat rotation, translation;
										cv::solvePnP(board->getObjectPoints(), toCv(imagePoints), cameraMatrix, distortionCoefficients, rotation, translation);

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
								const auto & view = camera->getViewInObjectSpace();
								view.beginAsCamera(true);
								glEnable(GL_DEPTH_TEST);
								glClear(GL_DEPTH_BUFFER_BIT);
								this->drawWorld();
								glDisable(GL_DEPTH_TEST);
								view.endAsCamera();
							}
							this->fbo.end();
						}
					}
				}
			}

			//----------
			void ARCube::populateInspector(ElementGroupPtr inspector) {
				auto activeWhenWidget = Widgets::MultipleChoice::make("Active");
				activeWhenWidget->addOption("When selected");
				activeWhenWidget->addOption("Always");
				activeWhenWidget->entangle(this->activewhen);
				inspector->add(activeWhenWidget);

				inspector->add(Widgets::Indicator::make("Found board", [this]() {
					if (this->getInputPin<Item::Board>()->isConnected() && this->getInputPin<Item::Camera>()->isConnected()) {
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
				inspector->add(Widgets::LiveValue<string>::make("Board position", [this]() {
					if (this->foundBoard) {
						stringstream ss;
						ss << (ofVec3f() * this->boardTransform);
						return ss.str();
					}
					else {
						return string("");
					}
				}));

				auto drawStyleSelector = Widgets::MultipleChoice::make("Draw style");
				drawStyleSelector->addOption("Axes");
				drawStyleSelector->addOption("Board");
				drawStyleSelector->addOption("Cube");
				drawStyleSelector->entangle(this->drawStyle);
				inspector->add(drawStyleSelector);
			}

			//----------
			void ARCube::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->activewhen, json);
				Utils::Serializable::serialize(this->drawStyle, json);
			}

			//----------
			void ARCube::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->activewhen, json);
				Utils::Serializable::deserialize(this->drawStyle, json);
			}

			//----------
			void ARCube::drawWorld() {
				auto camera = this->getInput<Item::Camera>();
				auto board = this->getInput<Item::Board>();
				if (board && camera && this->foundBoard) {
					ofPushStyle();
					{
						ofPushMatrix();
						{
							auto spacing = board->getSpacing();

							ofMultMatrix(camera->getTransform());
							ofMultMatrix(this->boardTransform);

							switch (this->drawStyle) {
							case 0:
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
							case 1:
							{
								board->drawObject();
								break;
							}
							case 2:
							{
								ofPushMatrix();
								{
									auto scale = board->getSpacing() * 4.0f;
									ofScale(scale, scale, scale);
									ofPushStyle();
									{
										ofFill();
										this->cube.draw();
									}
									ofPopStyle();
								}
								ofPopMatrix();
								break;
							}
							}
						}
						ofPopMatrix();
					}
					ofPopStyle();
				}
			}

			//----------
			bool ARCube::getRunFinderEnabled() const {
				if (this->activewhen == 1) {
					//find when we're set to always find
					return true;
				}
				else if (this->isBeingInspected()) {
					//find when we're selected
					return true;
				}
				else {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						auto grabber = camera->getGrabber();
						if (!grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_FreeRun)) {
							//find when the camera is a single shot camera
							return true;
						}
					}
				}

				//if nothing was good, then return false
				return false;
			}

			//----------
			ofxCvGui::PanelPtr ARCube::getView() {
				return this->view;
			}
		}
	}
}
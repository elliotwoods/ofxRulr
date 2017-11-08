#include "pch_Plugin_Scrap.h"
#include "LaserToWorld.h"

#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Capture
				//----------
				LaserToWorld::Capture::Capture() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				string LaserToWorld::Capture::getDisplayString() const {
					stringstream ss;
					ss << "World : " << ofToString(this->worldPosition.get(), 3) << endl;
					ss << "Projected : " << ofToString(this->projected.get(), 3);
					return ss.str();
				}

				//----------
				void LaserToWorld::Capture::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->worldPosition);
					Utils::Serializable::serialize(json, this->projected);
				}

				//----------
				void LaserToWorld::Capture::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->worldPosition);
					Utils::Serializable::deserialize(json, this->projected);
				}

#pragma mark LaserToWorld
				//----------
				LaserToWorld::LaserToWorld() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string LaserToWorld::getTypeName() const {
					return "Procedure::Calibrate::LaserToWorld";
				}

				//----------
				void LaserToWorld::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<Item::AbstractBoard>();
					this->addInput<Item::Projector>("Laser");

					//create the panel
					{
						auto panel = ofxCvGui::Panels::makeBlank();
						panel->onDraw += [this](ofxCvGui::DrawArguments & args) {
							ofPushMatrix();
							{
								ofTranslate(args.localBounds.width / 2.0f, args.localBounds.height / 2.0f);
								ofScale(args.localBounds.width / 2.0f, - args.localBounds.height / 2.0f);

								//draw grid
								ofPushStyle();
								{
									ofSetColor(100);
									for (float x = -1; x <= 1; x++) {
										ofDrawLine(x, -1, x, +1);
										ofDrawLine(-1, x, +1, x);
									}
								}
								ofPopStyle();

								//draw captures
								{
									auto captures = this->captures.getSelection();
									ofPushStyle();
									{
										for (const auto & capture : captures) {
											ofSetColor(capture->color);
											ofPushMatrix();
											{
												ofTranslate(capture->projected.get());
												ofDrawLine(-0.02, -0.02, 0.02, 0.02);
												ofDrawLine(-0.02, 0.02, 0.02, -0.02);
											}
											ofPopMatrix();
										}
									}
									ofPopStyle();
								}

								//draw current cursor
								{
									ofPushStyle();
									{
										ofPushMatrix();
										{
											ofTranslate(this->parameters.target.x, this->parameters.target.y);
											ofDrawLine(-0.02, -0.02, 0.02, 0.02);
											ofDrawLine(-0.02, 0.02, 0.02, -0.02);
										}
										ofPopMatrix();
									}
									ofPopStyle();
								}
							}
							ofPopMatrix();

						};

						panel->onMouse += [this](ofxCvGui::MouseArguments & args) {
							args.takeMousePress(this->panel); //this should be valid by the time it happens
							float speed = args.button == 2 ? 0.5f / 20.0f : 1.0f;
							if (args.action == ofxCvGui::MouseArguments::Action::Dragged) {
								this->parameters.target.x += args.movement.x / this->panel->getWidth() * speed;
								this->parameters.target.y -= args.movement.y / this->panel->getHeight() * speed;
								
								this->parameters.target.x = ofClamp(this->parameters.target.x, -1.0f, 1.0f);
								this->parameters.target.y = ofClamp(this->parameters.target.y, -1.0f, 1.0f);
							}
						};
						panel->onKeyboard += [this](ofxCvGui::KeyboardArguments & args) {
							if (this->isBeingInspected()) {
								ofVec2f movement;
								switch (args.key) {
								case OF_KEY_LEFT:
									movement.x = -0.5;
									break;
								case OF_KEY_RIGHT:
									movement.x = +0.5;
									break;
								case OF_KEY_UP:
									movement.y = -0.5;
									break;
								case OF_KEY_DOWN:
									movement.y = +0.5;
									break;
								default:
									break;
								}

								if (ofGetKeyPressed(OF_KEY_SHIFT)) {
									movement *= 20;
								}

								this->parameters.target.x += movement.x;
								this->parameters.target.y += movement.y;
							}
						};
						this->panel = panel;
					}
				}

				//----------
				void LaserToWorld::update() {
					//invalidate OSC sender 
					if (this->oscSender) {
						if (this->cachedRemoteHost != this->parameters.osc.remoteHost.get()
							|| this->cachedRemotePort != this->parameters.osc.remotePort.get()) {
							this->oscSender.reset();
						}
					}

					//create OSC sender
					if (!this->oscSender) {
						this->oscSender = make_unique<ofxOscSender>();
						this->oscSender->setup(this->parameters.osc.remoteHost, this->parameters.osc.remotePort);
						this->cachedRemoteHost = this->parameters.osc.remoteHost;
						this->cachedRemotePort = this->parameters.osc.remotePort;
					}

					//send the target position
					if (this->oscSender) {
						{
							ofxOscMessage message;
							message.setAddress("/laser/position");
							message.addFloatArg(this->parameters.target.x);
							message.addFloatArg(this->parameters.target.y);
							this->oscSender->sendMessage(message);
						}
						
						{
							ofxOscMessage message;
							message.setAddress("/laser/enabled");
							message.addInt32Arg(this->parameters.target.enabled.get() ? 1.0f : 0.0f);
							this->oscSender->sendMessage(message);
						}
					}
				}

				//----------
				void LaserToWorld::drawWorldStage() {
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						ofPushMatrix();
						{
							ofTranslate(capture->worldPosition);
							ofPushStyle();
							{
								ofSetColor(capture->color);
								ofDrawLine(0, -0.02, 0, +0.02);
								ofDrawLine(-0.02, 0, +0.02, 0);
								ofDrawBitmapString(ofToString(capture->projected, 3), 0, 0);
							}
							ofPopStyle();
						}
						ofPopMatrix();
					}
				}

				//----------
				ofxCvGui::PanelPtr LaserToWorld::getPanel() {
					return this->panel;
				}

				//----------
				void LaserToWorld::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);
					Utils::Serializable::serialize(json, this->reprojectionError);
					this->captures.serialize(json);
				}

				//----------
				void LaserToWorld::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);
					Utils::Serializable::deserialize(json, this->reprojectionError);
					this->captures.deserialize(json);
				}

				//----------
				void LaserToWorld::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					
					inspector->addParameterGroup(this->parameters);

					inspector->addButton("Add capture", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Add capture");
							this->addCapture();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, ' ');

					this->captures.populateWidgets(inspector);

					inspector->addButton("Calibrate", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Calibrate");
							this->calibrate();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

					inspector->addButton("Test", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Test");
							this->testCalibration();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, 't');


					inspector->addLiveValue<float>(this->reprojectionError);
				}

				//----------
				void LaserToWorld::addCapture() {
					if (!this->oscSender) {
						throw(ofxRulr::Exception("OSC sender not ready"));
					}

					//send message to off the laser
					{
						ofxOscMessage message;
						message.setAddress("/laser/enabled");
						message.addInt32Arg(0);
						this->oscSender->sendMessage(message);
					}

					//wait
					{
						this_thread::sleep_for(chrono::duration<double, ratio<1, 1>>(this->parameters.capture.outputDelay));
					}

					//find the position of the world marker
					{
						auto capture = make_shared<Capture>();
						capture->worldPosition = this->getWorldCursorPosition();
						capture->projected = ofVec2f(this->parameters.target.x
							, this->parameters.target.y);
						this->captures.add(capture);
					}
				}

				//----------
				void LaserToWorld::calibrate() {
					auto laser = this->getInput<Item::Projector>("Laser");
					if (!laser) {
						throw(ofxRulr::Exception("Missing connection to Laser"));
					}

					//faux dimensions
					laser->setWidth(1024.0f);
					laser->setHeight(1024.0f);

					vector<cv::Point2f> imagePoints;
					vector<cv::Point3f> worldPoints;

					auto captures = this->captures.getSelection();
					{
						for (const auto & capture : captures) {
							auto x = (capture->projected.get().x + 1.0f) / 2.0f * laser->getWidth();
							auto y = (1.0f - capture->projected.get().y) / 2.0f * laser->getHeight();

							imagePoints.emplace_back(x, y);
							worldPoints.push_back(ofxCv::toCv(capture->worldPosition));
						}
					}

					auto size = laser->getSize();
					cv::Mat cameraMatrix;
					cv::Mat distortionCoefficients;
					vector<cv::Mat> rotationVectors;
					vector<cv::Mat> translations;

					//initialise parameters
					{
						cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
						cameraMatrix.at<double>(0, 0) = size.width * this->parameters.calibrate.initialThrowRatio;
						cameraMatrix.at<double>(1, 1) = size.width * this->parameters.calibrate.initialThrowRatio;
						cameraMatrix.at<double>(0, 2) = size.width / 2.0f;
						cameraMatrix.at<double>(1, 2) = size.height * (0.50f - this->parameters.calibrate.initialLensOffset / 2.0f);
						distortionCoefficients = cv::Mat::zeros(5, 1, CV_64F);
					}
					

					auto flags = CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6
						| CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_USE_INTRINSIC_GUESS;
					if (this->parameters.calibrate.fixAspectRatio) {
						flags |= CV_CALIB_FIX_ASPECT_RATIO;
					}
					this->reprojectionError = cv::calibrateCamera(vector<vector<cv::Point3f>>(1, worldPoints)
						, vector<vector<cv::Point2f>>(1, imagePoints)
						, size
						, cameraMatrix
						, distortionCoefficients
						, rotationVectors
						, translations
						, flags);

					laser->setIntrinsics(cameraMatrix, distortionCoefficients);
					laser->setExtrinsics(rotationVectors[0], translations[0], true);
				}

				//----------
				void LaserToWorld::testCalibration() {
					this->throwIfMissingAConnection<Item::Projector>();
					auto laser = this->getInput<Item::Projector>("Laser");

					const auto worldPosition = this->getWorldCursorPosition();
					const auto view = laser->getViewInWorldSpace();
					auto coordinate = view.getNormalizedSCoordinateOfWorldPosition(worldPosition);

					if (coordinate.x >= -1.0f
						&& coordinate.x <= 1.0f
						&& coordinate.y >= -1.0f
						&& coordinate.y <= +1.0f) {
						this->parameters.target.x = coordinate.x;
						this->parameters.target.y = coordinate.y;
					}
					else {
						throw(ofxRulr::Exception("Can't get coordinate"));
					}
				}

				//----------
				ofVec3f LaserToWorld::getWorldCursorPosition() const {
					Utils::ScopedProcess scopedProcess("Get world cursor position", false);

					this->throwIfMissingAConnection<Item::Camera>();
					this->throwIfMissingAConnection<Item::AbstractBoard>();

					auto board = this->getInput<Item::AbstractBoard>();
					auto camera = this->getInput<Item::Camera>();

					auto frame = camera->getFreshFrame();
					if (!frame) {
						throw(ofxRulr::Exception("Couldn't capture frame from camera"));
					}

					vector<cv::Point2f> imagePoints;
					vector<cv::Point3f> objectPoints;
					board->findBoard(ofxCv::toCv(frame->getPixels())
						, imagePoints
						, objectPoints
						, this->parameters.capture.findBoardMode);

					cv::Mat rotationVector;
					cv::Mat translation;
					cv::solvePnP(objectPoints
						, imagePoints
						, camera->getCameraMatrix()
						, camera->getDistortionCoefficients()
						, rotationVector
						, translation);

					auto transform = ofxCv::makeMatrix(rotationVector, translation);
					ofVec3f objectPoint(this->parameters.capture.offsetSquares.get().x * board->getSpacing()
						, this->parameters.capture.offsetSquares.get().y * board->getSpacing()
						, 0.0f);
					auto worldPoint = objectPoint * transform;
					
					return worldPoint;
				}
			}
		}
	}
}
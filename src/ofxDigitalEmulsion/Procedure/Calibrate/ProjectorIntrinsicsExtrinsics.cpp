#include "ProjectorIntrinsicsExtrinsics.h"

#include "../../Item/Checkerboard.h"
#include "../../Item/Camera.h"
#include "../../Item/Projector.h"

#include "../../Utils/Exception.h"
#include "../../Utils/Utils.h"

#include "ofxCvGui.h"

using namespace ofxDigitalEmulsion::Graph;
using namespace ofxCvGui;

using namespace ofxCv;
using namespace cv;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//----------
			ProjectorIntrinsicsExtrinsics::ProjectorIntrinsicsExtrinsics() {
				this->inputPins.add(MAKE(Pin<Item::Camera>));
				this->inputPins.add(MAKE(Pin<Item::Projector>));
				this->inputPins.add(MAKE(Pin<Item::Checkerboard>));

				this->oscServer.setup(4005);

				this->fixAspectRatio.set("Fix aspect ratio", false);

				this->lastSeenFail = -10.0f;
				this->lastSeenSuccess = -10.0f;
				this->error = 0.0f;
			}

			//----------
			string ProjectorIntrinsicsExtrinsics::getTypeName() const {
				return "ProjectorIntrinsicsExtrinsics";
			}

			//----------
			Graph::PinSet ProjectorIntrinsicsExtrinsics::getInputPins() const {
				return this->inputPins;
			}

			//----------
			ofxCvGui::PanelPtr ProjectorIntrinsicsExtrinsics::getView() {
				auto view = MAKE(Panels::World);
				view->getCamera().rotate(180.0f, 0.0f, 0.0f, 1.0f);
				view->getCamera().lookAt(ofVec3f(0,0,1), ofVec3f(0,-1,0));
				view->onDrawWorld += [this] (ofCamera &) {
					for(auto & correspondence : this->correspondences) {
						ofPushMatrix();
						ofTranslate(correspondence.world);
						ofDrawAxis(0.05f);
						ofPopMatrix();
					}

					auto projector = this->getInput<Item::Projector>();
					if (projector) {
						projector->drawWorld();
					}

					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						camera->drawWorld();
					}
				};
				const auto flashDuration = 2.0f;
				view->onDraw += [this, flashDuration] (DrawArguments & drawArgs) {
					if (ofGetElapsedTimef() - this->lastSeenSuccess < flashDuration) {
						ofPushStyle();
						ofFill();
						ofSetColor(0, 255, 0, ofMap(ofGetElapsedTimef(), this->lastSeenSuccess, this->lastSeenSuccess + flashDuration, 255, 0.0f, true));
						ofRect(drawArgs.localBounds);
						ofPopStyle();
					}
					if (ofGetElapsedTimef() - this->lastSeenFail < flashDuration) {
						ofPushStyle();
						ofFill();
						ofSetColor(255, 0, 0, ofMap(ofGetElapsedTimef(), this->lastSeenFail, this->lastSeenFail + flashDuration, 255, 0.0f, true));
						ofRect(drawArgs.localBounds);
						ofPopStyle();
					}
				};
				return view;
			}

			//----------
			void ProjectorIntrinsicsExtrinsics::update() {
				bool processedThisFrame = false;
				while(this->oscServer.hasWaitingMessages()) {
					ofxOscMessage msg;
					if (!processedThisFrame) {
						oscServer.getNextMessage(& msg);
						if (msg.getAddress() == "/cursor" && msg.getNumArgs() >= 4) {
							this->addPoint(msg.getArgAsFloat(0), msg.getArgAsFloat(1), msg.getArgAsFloat(2), msg.getArgAsFloat(3));
						}
					} else {
						//clear all the messages out if we've already added this frame
						oscServer.getNextMessage(& msg);
					}
					processedThisFrame = true;
				}
			}
			
			//----------
			void ProjectorIntrinsicsExtrinsics::serialize(Json::Value & json) {
				for(int i=0; i<this->correspondences.size(); i++) {
					const auto & correspondence = this->correspondences[i];
					auto & jsonCorrespondence = json[i];
					auto & jsonWorld = jsonCorrespondence["world"];
					for(int j=0; j<3; j++) {
						jsonWorld[j] = correspondence.world[j];
					}
					auto & jsonProjector = jsonCorrespondence["projector"];
					for(int j=0; j<2; j++) {
						jsonProjector[j] = correspondence.projector[j];
					}
				}
			}

			//----------
			void ProjectorIntrinsicsExtrinsics::deserialize(const Json::Value & json) {
				this->correspondences.clear();
				for(auto & jsonCorrespondence : json) {
					auto & jsonWorld = jsonCorrespondence["world"];
					Correspondence correspondence;
					for(int j=0; j<3; j++) {
						correspondence.world[j] = jsonWorld[j].asFloat();
					}
					auto & jsonProjector = jsonCorrespondence["projector"];
					for(int j=0; j<2; j++) {
						correspondence.projector[j] = jsonProjector[j].asFloat();
					}
					this->correspondences.push_back(correspondence);
				}
			}

			//----------
			void ProjectorIntrinsicsExtrinsics::populateInspector2(ElementGroupPtr inspector) {
				inspector->add(Widgets::LiveValue<int>::make("Correspondence points", [this] () {
					return (int) this->correspondences.size();
				}));
				inspector->add(Widgets::Button::make("Clear correspondences", [this] () {
					this->correspondences.clear();
				}));
				auto calibrateButton = Widgets::Button::make("Calibrate", [this] () {
					try {
						this->calibrate();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				});
				calibrateButton->setHeight(100.0f);
				inspector->add(calibrateButton);
				inspector->add(Widgets::LiveValue<float>::make("Reprojection error [px]", [this] () {
					return this->error;
				}));
			}
			
			//----------
			void ProjectorIntrinsicsExtrinsics::calibrate() {
				this->throwIfMissingAnyConnection();
				auto camera = this->getInput<Item::Camera>();
				auto projector = this->getInput<Item::Projector>();

				vector<Point2f> imagePoints;
				vector<Point3f> worldPoints;

				for(const auto & correspondence : this->correspondences) {
					auto projectorPoint = toCv(correspondence.projector);
					projectorPoint.x = ofMap(projectorPoint.x, -1.0f, +1.0f, 0.0f, projector->getWidth());
					projectorPoint.y = ofMap(projectorPoint.y, +1.0f, -1.0f, 0.0f, projector->getHeight());
					imagePoints.push_back(projectorPoint);
					worldPoints.push_back(toCv(correspondence.world));
				}

				//we have to intitialise a basic camera matrix for it to start with (this will get changed by the function call calibrateCamera
				Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
				cameraMatrix.at<double>(0,0) = projector->getWidth() * 1.4f; // default at 1.4 : 1.0f throw ratio
				cameraMatrix.at<double>(1,1) = projector->getHeight() * 1.4f;
				cameraMatrix.at<double>(0,2) = projector->getWidth() / 2.0f;
				cameraMatrix.at<double>(1,2) = projector->getHeight() * 0.90f; // default at 40% lens offset

				//same again for distortion
				Mat distortionCoefficients = Mat::zeros(5, 1, CV_64F);

				vector<Mat> rotations, translations;

				int flags = CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6 | CV_CALIB_ZERO_TANGENT_DIST | CV_CALIB_USE_INTRINSIC_GUESS;
				if (this->fixAspectRatio) {
					flags |= CV_CALIB_FIX_ASPECT_RATIO;
				}

				this->error = cv::calibrateCamera(vector<vector<Point3f>>(1, worldPoints), vector<vector<Point2f>>(1, imagePoints), cv::Size(projector->getWidth(), projector->getHeight()), cameraMatrix, distortionCoefficients, rotations, translations, flags);
				
				projector->setExtrinsics(rotations[0], translations[0]);
				projector->setIntrinsics(cameraMatrix);
			}

			//----------
			void ProjectorIntrinsicsExtrinsics::addPoint(float projectorX, float projectorY, int projectorWidth, int projectorHeight) {
				auto camera = this->getInput<Item::Camera>();
				auto projector = this->getInput<Item::Projector>();
				auto checkerboard = this->getInput<Item::Checkerboard>();

				try {
					this->throwIfMissingAnyConnection();
					if (projectorWidth != projector->getWidth() || projectorHeight != projector->getHeight()) {
						stringstream message;
						message << "Resolution of cursor screen app [" << projectorWidth << "x" << projectorHeight << "] does not match Projector object that we are calibrating [" << projector->getWidth() << "x" << projector->getHeight() << "]";
						throw(Utils::Exception(message.str()));
					}

					vector<cv::Point2f> corners;
					ofxCvGui::Utils::drawProcessingNotice("Finding chessboard...");
					ofxCv::findChessboardCornersPreTest(toCv(camera->getGrabber()->getPixelsRef()), checkerboard->getSize(), corners, 1024);
					if (corners.empty()) {
						throw(Utils::Exception("No checkerboard found in image"));
					}
					Mat rotation;
					Mat translation;
					cv::solvePnP(checkerboard->getObjectPoints(), corners, camera->getCameraMatrix(), camera->getDistortionCoefficients(), rotation, translation, false);

					Correspondence newCorrespondence = {ofVec3f(translation.at<double>(0), translation.at<double>(1), translation.at<double>(2)), ofVec2f(projectorX, projectorY)};
					this->correspondences.push_back(newCorrespondence);
					this->lastSeenSuccess = ofGetElapsedTimef();
					Utils::playSuccessSound();
				} catch (const std::exception & e) {
					try {
						const auto & cvException = dynamic_cast<const cv::Exception &>(e);
						OFXDIGITALEMULSION_ERROR << cvException.msg;
					} catch (std::bad_cast) {
						OFXDIGITALEMULSION_ERROR << e.what();
					}
					this->lastSeenFail = ofGetElapsedTimef();
					Utils::playFailSound();
				}
			}
		}
	}
}
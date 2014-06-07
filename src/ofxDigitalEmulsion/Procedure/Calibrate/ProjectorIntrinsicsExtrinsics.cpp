#include "ProjectorIntrinsicsExtrinsics.h"

#include "../../Item/Checkerboard.h"
#include "../../Item/Camera.h"
#include "../../Item/Projector.h"

#include "../../Utils/Exception.h"

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
			}

			//----------
			string ProjectorIntrinsicsExtrinsics::getTypeName() const {
				return "ProjectorIntrinsicsExtrinsics";
			}

			//----------
			Graph::PinSet ProjectorIntrinsicsExtrinsics::getInputPins() {
				return this->inputPins;
			}

			//----------
			ofxCvGui::PanelPtr ProjectorIntrinsicsExtrinsics::getView() {
				auto view = MAKE(Panels::World);
				view->onDrawWorld += [this] (ofCamera &) {
					for(auto & correspondence : this->correspondences) {
						ofPushMatrix();
						ofTranslate(correspondence.world);
						ofDrawAxis(0.05f);
						ofPopMatrix();
					}
				};
				return view;
			}

			//----------
			void ProjectorIntrinsicsExtrinsics::update() {
				bool processedThisFrame = false;
				while(this->oscServer.hasWaitingMessages()) {
					if (!processedThisFrame) {
						ofxOscMessage msg;
						oscServer.getNextMessage(& msg);
						if (msg.getAddress() == "/cursor" && msg.getNumArgs() >= 4) {
							this->addPoint(msg.getArgAsFloat(0), msg.getArgAsFloat(1), msg.getArgAsFloat(2), msg.getArgAsFloat(3));
						}
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
						jsonWorld[j] = correspondence.world[i];
					}
					auto & jsonProjector = jsonCorrespondence["projector"];
					for(int j=0; j<2; j++) {
						jsonProjector[j] = correspondence.projector[i];
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
						correspondence.world[i] = jsonWorld[j];
					}
					auto & jsonProjector = jsonCorrespondence["projector"];
					for(int j=0; j<2; j++) {
						correspondence.projector[i] = jsonProjector[j];
					}
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
			}

			//----------
			void ProjectorIntrinsicsExtrinsics::addPoint(float projectorX, float projectorY, int projectorWidth, int projectorHeight) {
				auto camera = this->getInput<Item::Camera>();
				auto projector = this->getInput<Item::Projector>();
				auto checkerboard = this->getInput<Item::Checkerboard>();

				try {
					if (! camera) {
						throw(Utils::Exception("No camera attached"));
					}
					if (! projector) {
						throw(Utils::Exception("No projector attached"));
					}
					if (! checkerboard) {
						throw(Utils::Exception("No checkerboard attached"));
					}
					if (projectorWidth != projector->getWidth() || projectorHeight != projector->getHeight()) {
						stringstream message;
						message << "Resolution of cursor screen app [" << projectorWidth << "x" << projectorHeight << "] does not match Projector object that we are calibrating [" << projector->getWidth() << "x" << projector->getHeight() << "]";
						throw(Utils::Exception(message.str()));
					}

					cv::Mat grayscale;
					cv::cvtColor(toCv(camera->getGrabber()->getPixelsRef()), grayscale, CV_RGB2GRAY);
					vector<cv::Point2f> corners;
					cv::findChessboardCorners(grayscale, checkerboard->getSize(), corners);
					if (corners.empty()) {
						throw(Utils::Exception("No checkerboard found in image"));
					}
					Mat rotation;
					Mat translation;
					cv::solvePnP(checkerboard->getObjectPoints(), corners, camera->getCameraMatrix(), camera->getDistortionCoefficients(), rotation, translation, false);

					Correspondence newCorrespondence = {ofVec3f(translation.at<double>(0), translation.at<double>(1), translation.at<double>(2)), ofVec2f(projectorX, projectorY)};
					this->correspondences.push_back(newCorrespondence);

				} catch (const std::exception & e) {
					try {
						const auto & cvException = dynamic_cast<const cv::Exception &>(e);
						ofSystemAlertDialog(cvException.msg);
					} catch (std::bad_cast) {
						ofSystemAlertDialog(e.what());
					}
				}
			}
		}
	}
}
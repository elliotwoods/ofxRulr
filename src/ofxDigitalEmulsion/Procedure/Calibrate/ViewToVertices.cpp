#include "ViewToVertices.h"

#include "../../Item/View.h"
#include "../../Device/VideoOutput.h"
#include "IReferenceVertices.h"

#include "ofxCvGui/Widgets/SelectFile.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Button.h"
#include "ofxSpinCursor.h"

#include "ofxCvMin.h"

using namespace ofxCvGui;
using namespace ofxCv;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
#pragma mark ViewArea
			//---------
			ViewToVertices::ViewArea::ViewArea(ViewToVertices & parent) : parent(parent) {

			}

			//---------
			void ViewToVertices::ViewArea::draw(float x, float y) {
				this->draw(x, y, this->getWidth(), this->getHeight());
			}

			//---------
			void ViewToVertices::ViewArea::draw(float x, float y, float w, float h) {
				if (this->parent.projectorReferenceImage.isAllocated()) {
					this->parent.projectorReferenceImage.draw(x, y, w, h);
				}
			}

			//---------
			float ViewToVertices::ViewArea::getHeight() {
				auto view = this->parent.getInput<Item::View>();
				if (view) {
					return view->getHeight();
				}
				else {
					return 1;
				}
			}

			//---------
			float ViewToVertices::ViewArea::getWidth() {
				auto view = this->parent.getInput<Item::View>();
				if (view) {
					return view->getWidth();
				}
				else {
					return 1;
				}
			}

#pragma mark ViewToVertices
			//---------
			ViewToVertices::ViewToVertices() : viewArea(*this) {
				OFXDIGITALEMULSION_NODE_INIT_LISTENER;
			}

			//---------
			string ViewToVertices::getTypeName() const {
				return "Procedure::Calibrate::ViewToVertices";
			}

			//---------
			void ViewToVertices::init() {
				OFXDIGITALEMULSION_NODE_UPDATE_LISTENER;
				OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
				OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::View>();
				auto videoOutputPin = this->addInput<Device::VideoOutput>();
				auto referenceVerticesPin = this->addInput<IReferenceVertices>();

				auto view = make_shared<Panels::Draws>(this->viewArea);
				auto viewWeak = weak_ptr<Panels::Draws>(view);
				view->onDrawCropped += [this](Panels::Draws::DrawCroppedArguments & args) {
					//draw a label at each vertex
					auto referenceVertices = this->getInput<IReferenceVertices>();
					if (!referenceVertices) {
						return;
					}
					auto scaleDrawToScreenFactor = args.drawSize / args.viewSize;

					ofPushMatrix();
					const auto & vertices = referenceVertices->getVertices();
					int index = 0;
					for (auto vertex : vertices) {
						ofPushMatrix();
						ofTranslate(vertex->viewPosition);

						//text label
						ofDrawBitmapString(ofToString(index++), 0, 0);

						//cross
						ofPushStyle();
						ofScale(scaleDrawToScreenFactor.x, scaleDrawToScreenFactor.y);
						//surround
						ofSetLineWidth(3.0f);
						ofSetColor(0);
						ofLine(-10, 0, 10, 0);
						ofLine(0, -10, 0, 10);
						//inner
						ofSetLineWidth(1.0f);
						ofSetColor(vertex->isSelected() ? ofxCvGui::Utils::getBeatingSelectionColor() : ofColor(100));
						ofLine(-10, 0, 10, 0);
						ofLine(0, -10, 0, 10);
						ofPopStyle();

						ofPopMatrix();
					}
					ofPopMatrix();
				};
				view->onMouse += [this, viewWeak](ofxCvGui::MouseArguments & args) {
					auto view = viewWeak.lock();
					args.takeMousePress(view);

					//if the mouse is dragging then move the vertex in view space
					if (args.isDragging(view) && this->dragVerticesEnabled) {
						auto videoOutput = this->getInput<Device::VideoOutput>();
						auto referenceVertices = this->getInput<IReferenceVertices>();
						if (videoOutput && referenceVertices) {
							const auto & vertices = referenceVertices->getVertices();
							for (auto vertex : vertices) {
								if (vertex->isSelected()) {
									auto multiplier = 0.5f;
									if (ofGetKeyPressed(OF_KEY_SHIFT)) {
										multiplier /= 5.0f;
									}
									vertex->viewPosition += args.movement * multiplier;
									referenceVertices->onChangeVertex.notifyListeners();
								}
							}
						}
					}
				};
				view->onKeyboard += [this](ofxCvGui::KeyboardArguments & args) {
					ofVec2f movement;
					switch (args.key) {
					case OF_KEY_LEFT:
						movement.x = -1;
						break;
					case OF_KEY_RIGHT:
						movement.x = +1;
						break;
					case OF_KEY_UP:
						movement.y = -1;
						break;
					case OF_KEY_DOWN:
						movement.y = +1;
						break;
					}
					if (movement != ofVec2f()) {
						if (ofGetKeyPressed(OF_KEY_SHIFT)) {
							movement *= 5;
						}
						auto referenceVertices = this->getInput<IReferenceVertices>();
						if (referenceVertices) {
							const auto & vertices = referenceVertices->getVertices();
							for (auto vertex : vertices) {
								if (vertex->isSelected()) {
									vertex->viewPosition += movement;
									referenceVertices->onChangeVertex.notifyListeners();
								}
							}
						}
					}
				};
				this->view = view;

				this->dragVerticesEnabled.set("Drag vertices enabled", true);
				this->projectorReferenceImageFilename.set("Projector reference image filename", "");
				this->calibrateOnVertexChange.set("Calibrate on vertex change", true);

				videoOutputPin->onNewConnectionTyped += [this](shared_ptr<Device::VideoOutput> videoOutput) {
					videoOutput->onDrawOutput.addListener([this](ofRectangle & outputRectangle) {
						this->drawOnProjector();
					}, this);
				};
				videoOutputPin->onDeleteConnectionTyped += [this](shared_ptr<Device::VideoOutput> videoOutput) {
					if (videoOutput) {
						videoOutput->onDrawOutput.removeListeners(this);
					}
				};

				//auto-call calibrate when a vertex changes
				referenceVerticesPin->onNewConnectionTyped += [this](shared_ptr<IReferenceVertices> referenceVertices) {
					auto referenceVerticesWeak = weak_ptr<IReferenceVertices>(referenceVertices);
					referenceVertices->onChangeVertex.addListener([this, referenceVerticesWeak]() {
						if (this->calibrateOnVertexChange) {
							auto referenceVertices = referenceVerticesWeak.lock();
							//require at least 5 vertices
							if (referenceVertices && referenceVertices->getVertices().size() >= 5) {
								try {
									this->calibrate();
								}
								OFXDIGITALEMULSION_CATCH_ALL_TO_ERROR
							}
						}
					}, this);
				};
				referenceVerticesPin->onDeleteConnectionTyped += [this](shared_ptr<IReferenceVertices> referenceVertices) {
					referenceVertices->onChangeVertex.removeListeners(this);
				};
			}

			//---------
			ofxCvGui::PanelPtr ViewToVertices::getView() {
				return this->view;
			}

			//---------
			void ViewToVertices::update() {
				
			}

			//---------
			void ViewToVertices::serialize(Json::Value & json) {
				ofxDigitalEmulsion::Utils::Serializable::serialize(this->projectorReferenceImageFilename, json);
			}

			//---------
			void ViewToVertices::deserialize(const Json::Value & json) {
				ofxDigitalEmulsion::Utils::Serializable::deserialize(this->projectorReferenceImageFilename, json);
				if (this->projectorReferenceImageFilename.get().empty()) {
					this->projectorReferenceImage.clear();
				}
				else {
					this->projectorReferenceImage.loadImage(this->projectorReferenceImageFilename.get());
				}
			}

			//---------
			void ViewToVertices::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				inspector->add(Widgets::Toggle::make(this->dragVerticesEnabled));

				inspector->add(Widgets::Title::make("Reference image", Widgets::Title::Level::H3));
				inspector->add(Widgets::SelectFile::make(this->projectorReferenceImageFilename.getName(), [this](){
					return this->projectorReferenceImageFilename.get();
				}, [this](string & newFilename) {
					this->projectorReferenceImageFilename = newFilename;
					this->projectorReferenceImage.loadImage(newFilename);
				}));
				inspector->add(Widgets::Button::make("Clear iamge", [this]() {
					this->projectorReferenceImage.clear();
					this->projectorReferenceImageFilename.set("");
				}));
				
				inspector->add(Widgets::Spacer::make());

				inspector->add(Widgets::Title::make("Calibrate", Widgets::Title::Level::H3));
				auto calibrateButton = Widgets::Button::make("Calibrate", [this]() {
					try {
						this->calibrate();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				calibrateButton->setHeight(100.0f);
				inspector->add(calibrateButton);
				inspector->add(Widgets::Toggle::make(this->calibrateOnVertexChange));
				inspector->add(Widgets::LiveValue<float>::make("Reprojection error", [this]() {
					return this->reprojectionError;
				}));
				inspector->add(Widgets::Indicator::make("Calibration success", [this]() {
					return (Widgets::Indicator::Status) this->success;
				}));

				this->success = false;
				this->reprojectionError = 0.0f;
			}

			//---------
			void ViewToVertices::calibrate() {
				this->success = false;

				this->throwIfMissingAConnection<IReferenceVertices>();
				this->throwIfMissingAConnection<Item::View>();

				auto verticesNode = this->getInput<IReferenceVertices>();
				auto viewNode = this->getInput<Item::View>();

				const auto & vertices = verticesNode->getVertices();

				auto worldRows = vector<vector<ofVec3f>>(1);
				auto viewRows = vector<vector<ofVec2f>>(1);
				auto & world = worldRows[0];
				auto & view = viewRows[0]; 
				for (auto vertex : vertices) {
					world.push_back(vertex->getWorldPosition());
					view.push_back(vertex->viewPosition);
				}

				auto cameraMatrix = viewNode->getCameraMatrix();
				auto distortionCoefficients = viewNode->getDistortionCoefficients();
				vector<cv::Mat> rotations, translations;
				auto viewSize = viewNode->getSize();

				//clamp the initial intrinsics so that principal point is inside the image plane.
				//this is generally true for cameras, and insisted by OpenCV before fitting,
				//but often not true for projectors and the fit function will return principal
				//points outside of this range.
				auto & principalPointX = cameraMatrix.at<double>(0, 2);
				auto & principalPointY = cameraMatrix.at<double>(1, 2);
				if (principalPointX <= 1) {
					principalPointX = 1;
				}
				if (principalPointX >= viewSize.width - 2) {
					principalPointX = viewSize.width - 2;
				}
				if (principalPointY <= 1) {
					principalPointY = 1;
				}
				if (principalPointY >= viewSize.height - 2) {
					principalPointY = viewSize.height - 2;
				}

				auto flags = CV_CALIB_USE_INTRINSIC_GUESS; // since we're using a single object
				if (viewNode->getHasDistortion()) {
					flags |= OFXDIGITALEMULSION_VIEW_CALIBRATION_FLAGS;
				}
				else {
					flags |= (CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6 | CV_CALIB_ZERO_TANGENT_DIST);
				}
				auto reprojectionError = cv::calibrateCamera(toCv(worldRows), toCv(viewRows), viewSize, cameraMatrix, distortionCoefficients, rotations, translations, flags);
				//we might have thrown at this point
				
				viewNode->setIntrinsics(cameraMatrix, distortionCoefficients);
				auto objectTransform = ofxCv::makeMatrix(rotations[0], translations[0]);
				viewNode->setTransform(objectTransform.getInverse());
				this->success = true;
				this->reprojectionError = reprojectionError;
			}

			//---------
			void ViewToVertices::drawOnProjector() {
				auto referenceVerticesNode = this->getInput<IReferenceVertices>();
				if (referenceVerticesNode) {
					auto vertices = referenceVerticesNode->getVertices();
					for (auto vertex : vertices) {
						if (vertex->isSelected()){
							ofxSpinCursor::draw(vertex->viewPosition);
						}
					}
				}
			}
		}
	}
}
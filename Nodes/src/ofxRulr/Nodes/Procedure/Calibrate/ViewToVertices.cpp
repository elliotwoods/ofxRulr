#include "pch_RulrNodes.h"
#include "ViewToVertices.h"

#include "ofxRulr/Nodes/Item/View.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"
#include "IReferenceVertices.h"

#include "ofxCvGui/Widgets/SelectFile.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Button.h"
#include "../../../addons/ofxSpinCursor/src/ofxSpinCursor.h"

#include "ofxCvMin.h"

using namespace ofxCvGui;
using namespace ofxCv;
using namespace ofxRulr::Nodes;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark ViewArea
				//---------
				ViewToVertices::ViewArea::ViewArea(ViewToVertices & parent) : parent(parent) {

				}

				//---------
				void ViewToVertices::ViewArea::draw(float x, float y) const {
					this->draw(x, y, this->getWidth(), this->getHeight());
				}

				//---------
				void ViewToVertices::ViewArea::draw(float x, float y, float w, float h) const {
					if (this->parent.projectorReferenceImage.isAllocated()) {
						this->parent.projectorReferenceImage.draw(x, y, w, h);
					}
				}

				//---------
				float ViewToVertices::ViewArea::getHeight() const {
					auto view = this->parent.getInput<Item::View>();
					if (view) {
						return view->getHeight();
					}
					else {
						return 1;
					}
				}

				//---------
				float ViewToVertices::ViewArea::getWidth() const {
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
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string ViewToVertices::getTypeName() const {
					return "Procedure::Calibrate::ViewToVertices";
				}

				//---------
				void ViewToVertices::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::View>();
					auto videoOutputPin = this->addInput<System::VideoOutput>();
					auto referenceVerticesPin = this->addInput<IReferenceVertices>();

					auto view = make_shared<Panels::Draws>(this->viewArea);
					auto viewWeak = weak_ptr<Panels::Draws>(view);
					view->onDrawCropped += [this](DrawCroppedArguments & args) {
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
							ofDrawLine(-10, 0, 10, 0);
							ofDrawLine(0, -10, 0, 10);
							//inner
							ofSetLineWidth(1.0f);
							ofSetColor(vertex->isSelected() ? ofxCvGui::Utils::getBeatingSelectionColor() : ofColor(100));
							ofDrawLine(-10, 0, 10, 0);
							ofDrawLine(0, -10, 0, 10);
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
							auto referenceVertices = this->getInput<IReferenceVertices>();
							if (referenceVertices) {
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
						if (args.action == KeyboardArguments::Action::Pressed) {
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
							default:
								break;
							}

							if (movement != ofVec2f() && this->dragVerticesEnabled) {
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
						}
					};

					this->view = view;

					this->dragVerticesEnabled.set("Drag vertices enabled", true);
					this->useExistingParametersAsInitial.set("Use existing data as initial", false);
					this->projectorReferenceImageFilename.set("Projector reference image filename", "");
					this->calibrateOnVertexChange.set("Calibrate on vertex change", true);

					videoOutputPin->onNewConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						videoOutput->onDrawOutput.addListener([this](ofRectangle & outputRectangle) {
							this->drawOnProjector();
						}, this);
					};
					videoOutputPin->onDeleteConnection += [this](shared_ptr<System::VideoOutput> videoOutput) {
						if (videoOutput) {
							videoOutput->onDrawOutput.removeListeners(this);
						}
					};

					//auto-call calibrate when a vertex changes
					referenceVerticesPin->onNewConnection += [this](shared_ptr<IReferenceVertices> referenceVertices) {
						auto referenceVerticesWeak = weak_ptr<IReferenceVertices>(referenceVertices);
						referenceVertices->onChangeVertex.addListener([this, referenceVerticesWeak]() {
							if (this->calibrateOnVertexChange) {
								auto referenceVertices = referenceVerticesWeak.lock();
								//require at least 5 vertices
								if (referenceVertices && referenceVertices->getVertices().size() >= 5) {
									try {
										this->calibrate();
									}
									RULR_CATCH_ALL_TO_ERROR
								}
							}
						}, this);
					};
					referenceVerticesPin->onDeleteConnection += [this](shared_ptr<IReferenceVertices> referenceVertices) {
						referenceVertices->onChangeVertex.removeListeners(this);
					};

					this->success = false;
					this->reprojectionError = 0.0f;
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
					ofxRulr::Utils::Serializable::serialize(this->projectorReferenceImageFilename, json);
					ofxRulr::Utils::Serializable::serialize(this->dragVerticesEnabled, json);
					ofxRulr::Utils::Serializable::serialize(this->calibrateOnVertexChange, json);
					ofxRulr::Utils::Serializable::serialize(this->useExistingParametersAsInitial, json);
				}

				//---------
				void ViewToVertices::deserialize(const Json::Value & json) {
					ofxRulr::Utils::Serializable::deserialize(this->projectorReferenceImageFilename, json);
					if (this->projectorReferenceImageFilename.get().empty()) {
						this->projectorReferenceImage.clear();
					}
					else {
						this->projectorReferenceImage.load(this->projectorReferenceImageFilename.get());
					}

					ofxRulr::Utils::Serializable::deserialize(this->dragVerticesEnabled, json);
					ofxRulr::Utils::Serializable::deserialize(this->calibrateOnVertexChange, json);
					ofxRulr::Utils::Serializable::deserialize(this->useExistingParametersAsInitial, json);
				}

				//---------
				void ViewToVertices::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					inspector->add(new Widgets::Toggle(this->dragVerticesEnabled));

					inspector->add(new Widgets::Title("Reference image", Widgets::Title::Level::H3));
					inspector->add(new Widgets::SelectFile(this->projectorReferenceImageFilename.getName(), [this](){
						return this->projectorReferenceImageFilename.get();
					}, [this](string & newFilename) {
						this->projectorReferenceImageFilename = newFilename;
						this->projectorReferenceImage.load(newFilename);
					}));
					inspector->add(new Widgets::Button("Clear iamge", [this]() {
						this->projectorReferenceImage.clear();
						this->projectorReferenceImageFilename.set("");
					}));

					inspector->add(new Widgets::Spacer());

					inspector->add(new Widgets::Title("Calibrate", Widgets::Title::Level::H3));

					auto calibrateButton = new Widgets::Button("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT
					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);

					inspector->add(new Widgets::Toggle(this->calibrateOnVertexChange));
					inspector->add(new Widgets::Toggle(this->useExistingParametersAsInitial));
					inspector->add(new Widgets::LiveValue<float>("Reprojection error", [this]() {
						return this->reprojectionError;
					}));
					inspector->add(new Widgets::Indicator("Calibration success", [this]() {
						return (Widgets::Indicator::Status) this->success;
					}));
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

					vector<cv::Mat> rotations, translations;
					auto viewSize = viewNode->getSize();



					//--
					//Initialise matrices
					//--
					//
					cv::Mat cameraMatrix, distortionCoefficients;
					if (this->useExistingParametersAsInitial) {
						cameraMatrix = viewNode->getCameraMatrix();
						distortionCoefficients = viewNode->getDistortionCoefficients();

						//clamp the initial intrinsics so that focal length is positive
						auto & focalLengthX = cameraMatrix.at<double>(0, 0);
						auto & focalLengthY = cameraMatrix.at<double>(1, 1);
						if (focalLengthX <= 0) {
							focalLengthX = viewSize.width;
						}
						if (focalLengthY <= 0) {
							focalLengthY = viewSize.height;
						}

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
					}
					else {
						//setup some default characteristics
						cameraMatrix = Mat::eye(3, 3, CV_64F);
						cameraMatrix.at<double>(0, 0) = (double)viewSize.width * 1.0; // default at throw ratio of 1. : 1.0f throw ratio
						cameraMatrix.at<double>(1, 1) = (double)viewSize.height * 1.0;
						cameraMatrix.at<double>(0, 2) = viewSize.width / 2.0f;
						cameraMatrix.at<double>(1, 2) = viewSize.height * (0.50f - -0.4 / 2.0f); // default at 40% lens offset

						distortionCoefficients = Mat::zeros(5, 1, CV_64F);
					}
					//
					//--


					//--
					//check videoOutput has same size
					//--
					//
					auto videoOutput = this->getInput<System::VideoOutput>();
					if (videoOutput) {
						if (videoOutput->getWidth() != viewSize.width || videoOutput->getHeight() != viewSize.height) {
							ofLogWarning("ViewToVertices") << "VideoOutput's size does not match View's size";
						}
					}
					//
					//--



					//INSERT SYNETHESISED DATA
					//ofLogWarning() << "USING SYNTHESISED DATA for 1280x800 view";
					//worldRows.clear(); viewRows.clear();
					//worldRows.push_back(vector<ofVec3f>()); viewRows.push_back(vector<ofVec2f>());
					//worldRows[0].push_back(ofVec3f(0.4245, -0.3661, 0.4878)); viewRows[0].push_back(ofVec2f(596.9032, 721.8095));
					//worldRows[0].push_back(ofVec3f(0.3507, 0.4697, -0.4054)); viewRows[0].push_back(ofVec2f(490.7527, 278.5092));
					//worldRows[0].push_back(ofVec3f(-0.4230, 0.1083, -0.4126)); viewRows[0].push_back(ofVec2f(839.1771, 427.4655));
					//worldRows[0].push_back(ofVec3f(-0.1699, -0.0651, -0.4814)); viewRows[0].push_back(ofVec2f(709.3167, 524.0535));
					//worldRows[0].push_back(ofVec3f(0.2611, 0.3133, -0.1917)); viewRows[0].push_back(ofVec2f(563.7585, 382.7776));
					//worldRows[0].push_back(ofVec3f(0.1478, 0.3996, 0.4931)); viewRows[0].push_back(ofVec2f(689.3846, 419.8475));
					//worldRows[0].push_back(ofVec3f(0.0032, 0.1185, -0.2893)); viewRows[0].push_back(ofVec2f(656.1019, 455.0570));

					//--
					//setup flags
					//--
					//
					auto flags = CV_CALIB_USE_INTRINSIC_GUESS; // since we're using a single object
					if (viewNode->getHasDistortion()) {
						flags |= RULR_VIEW_CALIBRATION_FLAGS;
					}
					else {
						flags |= (CV_CALIB_FIX_K1 | CV_CALIB_FIX_K2 | CV_CALIB_FIX_K3 | CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6 | CV_CALIB_ZERO_TANGENT_DIST);
					}
					//--



					//--
					//FIT
					//--
					//
					auto reprojectionError = cv::calibrateCamera(toCv(worldRows), toCv(viewRows), viewSize, cameraMatrix, distortionCoefficients, rotations, translations, flags);
					//we might have thrown at this point
					//
					//--



					//--
					//Set output
					//--
					//
					viewNode->setIntrinsics(cameraMatrix, distortionCoefficients);
					auto objectTransform = ofxCv::makeMatrix(rotations[0], translations[0]);
					viewNode->setTransform(objectTransform.getInverse());
					this->success = true;
					this->reprojectionError = reprojectionError;
					//
					//--

					for (auto & point : worldRows[0]) {
						cout << point << endl;
					}
					for (auto & point : viewRows[0]) {
						cout << point << endl;
					}
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
}
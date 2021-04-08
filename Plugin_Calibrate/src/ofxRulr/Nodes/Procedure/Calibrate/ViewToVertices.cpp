#include "pch_Plugin_Calibrate.h"
#include "ViewToVertices.h"

#include "ofxRulr/Nodes/Item/View.h"
#include "ofxRulr/Nodes/System/VideoOutput.h"
#include "IReferenceVertices.h"

#include "ofxCvGui/Widgets/SelectFile.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Button.h"
#include "ofxSpinCursor.h"

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
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->addInput<Item::View>();
					auto videoOutputPin = this->addInput<System::VideoOutput>();
					auto referenceVerticesPin = this->addInput<IReferenceVertices>();

					auto view = make_shared<Panels::Draws>(this->viewArea);
					auto viewWeak = weak_ptr<Panels::Draws>(view);
					view->onDrawImage += [this](DrawImageArguments & args) {
						ofPushStyle();
						{
							ofNoFill();
							ofSetLineWidth(1.0f);
							ofDrawRectangle(args.drawBounds);
						}
						ofPopStyle();

						//draw a label at each vertex
						auto referenceVertices = this->getInput<IReferenceVertices>();
						if (!referenceVertices) {
							return;
						}
						auto scaleDrawToScreenFactor = args.drawBounds.getBottomRight() / args.drawArguments.localBounds.getBottomRight();

						ofPushMatrix();
						{
							const auto vertices = referenceVertices->getSelectedVertices();
							int index = 0;
							for (auto vertex : vertices) {
								ofPushMatrix();
								{
									ofTranslate(vertex->viewPosition.get());

									//text label
									auto caption = ofToString(index);
									ofDrawBitmapString(caption, 0, 0);

									//cross
									ofPushStyle();
									{
										ofScale(scaleDrawToScreenFactor.x, scaleDrawToScreenFactor.y);

										//surround
										ofSetLineWidth(3.0f);
										ofSetColor(0);
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);

										//inner
										ofSetLineWidth(1.0f);
										ofSetColor(ofColor(100));
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);
									}
									ofPopStyle();
								}
								ofPopMatrix();

								auto selection = this->selection.lock();
								if (selection) {
									ofPushStyle();
									{
										ofNoFill();
										ofSetLineWidth(2.0f);
										ofSetColor(ofxCvGui::Utils::getBeatingSelectionColor());

										ofPushMatrix();
										{
											ofTranslate(selection->viewPosition.get());
											ofScale(scaleDrawToScreenFactor.x, scaleDrawToScreenFactor.y);
											ofDrawCircle(glm::vec2(), 10.0f);
										}
										ofPopMatrix();
									}
									ofPopStyle();
								}
								index++;
							}
						}
						ofPopMatrix();
					};
					view->onMouse.addListener([this, viewWeak](ofxCvGui::MouseArguments & args) {
						if (this->dragVerticesEnabled) {
							auto view = viewWeak.lock();
							args.takeMousePress(view);

							//if the mouse is dragging then move the vertex in view space
							if (args.isDragging(view)) {
								auto selection = this->selection.lock();
								if (selection) {
									auto multiplier = 0.5f;
									if (ofGetKeyPressed(OF_KEY_SHIFT)) {
										multiplier /= 5.0f;
									}
									selection->viewPosition += args.movement * multiplier;
									selection->onChange.notifyListeners();
								}
							}
						}
					}, this, -1);
					view->onKeyboard += [this](ofxCvGui::KeyboardArguments & args) {
						if (args.action == KeyboardArguments::Action::Pressed) {
							auto referenceVertices = this->getInput<IReferenceVertices>();
							if (!referenceVertices) {
								return;
							}

							auto vertex = this->selection.lock();

							glm::vec2 movement;
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
							case OF_KEY_TAB:
								this->selection = referenceVertices->getNextVertex(vertex, ofGetKeyPressed(OF_KEY_SHIFT) ? -1 : 1);
								break;
							default:
								break;
							}

							if (vertex && movement != glm::vec2() && this->isBeingInspected()) {
								if (ofGetKeyPressed(OF_KEY_SHIFT)) {
									movement *= 20;
								}
								vertex->viewPosition += movement;
								vertex->onChange.notifyListeners();
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

					referenceVerticesPin->onNewConnection += [this](shared_ptr<IReferenceVertices> referenceVertices) {
						auto referenceVerticesWeak = weak_ptr<IReferenceVertices>(referenceVertices);
						referenceVertices->onChangeVertex.addListener([this, referenceVerticesWeak]() {
							if (this->calibrateOnVertexChange) {
								//auto-call calibrate when a vertex changes

								auto referenceVertices = referenceVerticesWeak.lock();
								try {
									this->calibrate();
								}
								catch (...) {
									//ignore all errors
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
				ofxCvGui::PanelPtr ViewToVertices::getPanel() {
					return this->view;
				}

				//---------
				void ViewToVertices::update() {
					auto referenceVertices = this->getInput<IReferenceVertices>();
					if (!referenceVertices) {
						//if we're unplugged, reset
						this->selection.reset();
					}
					else {
						auto availableVertices = referenceVertices->getSelectedVertices();
						auto selection = this->selection.lock();
						if (availableVertices.empty()) {
							//if there's no vertices, reset
							this->selection.reset();
						}
						else {
							//check the vertex we have is in the set, otherwise pick first
							auto findVertex = std::find(availableVertices.begin(), availableVertices.end(), selection);
							if (findVertex == availableVertices.end() || !selection) {
								this->selection = availableVertices[0];
							}
						}
					}
				}

				//---------
				void ViewToVertices::serialize(nlohmann::json & json) {
					ofxRulr::Utils::serialize(json, this->projectorReferenceImageFilename);
					ofxRulr::Utils::serialize(json, this->dragVerticesEnabled);
					ofxRulr::Utils::serialize(json, this->calibrateOnVertexChange);
					ofxRulr::Utils::serialize(json, this->useExistingParametersAsInitial);
				}

				//---------
				void ViewToVertices::deserialize(const nlohmann::json & json) {
					ofxRulr::Utils::deserialize(json, this->projectorReferenceImageFilename);
					if (this->projectorReferenceImageFilename.get().empty()) {
						this->projectorReferenceImage.clear();
					}
					else {
						this->projectorReferenceImage.load(this->projectorReferenceImageFilename.get());
					}

					ofxRulr::Utils::deserialize(json, this->dragVerticesEnabled);
					ofxRulr::Utils::deserialize(json, this->calibrateOnVertexChange);
					ofxRulr::Utils::deserialize(json, this->useExistingParametersAsInitial);
				}

				//---------
				void ViewToVertices::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
					auto inspector = inspectArguments.inspector;
					
					inspector->add(new Widgets::Toggle(this->dragVerticesEnabled));

					inspector->add(new Widgets::Title("Reference image", Widgets::Title::Level::H3));
					{
						auto widget = new Widgets::SelectFile(this->projectorReferenceImageFilename);
						widget->onValueChange += [this](const filesystem::path & path) {
							this->projectorReferenceImage.load(path.string());
						};

						inspector->add(widget);
					}
					inspector->add(new Widgets::Button("Clear image", [this]() {
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

					const auto & vertices = verticesNode->getSelectedVertices();

					auto worldRows = vector<vector<glm::vec3>>(1);
					auto viewRows = vector<vector<glm::vec2>>(1);
					auto & world = worldRows[0];
					auto & view = viewRows[0];
					for (auto vertex : vertices) {
						world.push_back(vertex->worldPosition);
						view.push_back(vertex->viewPosition);
					}

					vector<cv::Mat> rotations, translations;
					
					auto videoOutputNode = this->getInput<System::VideoOutput>();
					if (videoOutputNode && videoOutputNode->isWindowOpen()) {
						viewNode->setWidth(videoOutputNode->getWidth());
						viewNode->setHeight(videoOutputNode->getHeight());
					}

					auto viewSize = viewNode->getSize();



					//--
					//Initialize matrices
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
					//worldRows.push_back(vector<glm::vec3>()); viewRows.push_back(vector<glm::vec2>());
					//worldRows[0].push_back(glm::vec3(0.4245, -0.3661, 0.4878)); viewRows[0].push_back(glm::vec2(596.9032, 721.8095));
					//worldRows[0].push_back(glm::vec3(0.3507, 0.4697, -0.4054)); viewRows[0].push_back(glm::vec2(490.7527, 278.5092));
					//worldRows[0].push_back(glm::vec3(-0.4230, 0.1083, -0.4126)); viewRows[0].push_back(glm::vec2(839.1771, 427.4655));
					//worldRows[0].push_back(glm::vec3(-0.1699, -0.0651, -0.4814)); viewRows[0].push_back(glm::vec2(709.3167, 524.0535));
					//worldRows[0].push_back(glm::vec3(0.2611, 0.3133, -0.1917)); viewRows[0].push_back(glm::vec2(563.7585, 382.7776));
					//worldRows[0].push_back(glm::vec3(0.1478, 0.3996, 0.4931)); viewRows[0].push_back(glm::vec2(689.3846, 419.8475));
					//worldRows[0].push_back(glm::vec3(0.0032, 0.1185, -0.2893)); viewRows[0].push_back(glm::vec2(656.1019, 455.0570));

					//--
					//setup flags
					//--
					//
					auto flags = (int) cv::CALIB_USE_INTRINSIC_GUESS; // since we're using a single object
					if (viewNode->getHasDistortion()) {
						flags |= RULR_VIEW_CALIBRATION_FLAGS;
					}
					else {
						flags |= (cv::CALIB_FIX_K1 | cv::CALIB_FIX_K2 | cv::CALIB_FIX_K3 | cv::CALIB_FIX_K4 | cv::CALIB_FIX_K5 | cv::CALIB_FIX_K6 | cv::CALIB_ZERO_TANGENT_DIST);
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
					viewNode->setTransform(glm::inverse(objectTransform));
					this->success = true;
					this->reprojectionError = reprojectionError;
					//
					//--
				}

				//---------
				void ViewToVertices::drawOnProjector() {
					auto selection = this->selection.lock();
					if (selection) {
						ofxSpinCursor::draw(selection->viewPosition);
					}
				}

				//---------
				void ViewToVertices::drawWorldStage() {
					auto selection = this->selection.lock();
					if (selection) {
						//draw circle around point, facing towards camera
						ofPushMatrix();
						{
							auto & camera = ofxRulr::Graph::World::X().getWorldStage()->getCamera();
							auto transform = glm::lookAt(camera.getPosition()
								, selection->worldPosition.get()
								, glm::vec3(0, 1, 0));

							ofMultMatrix(transform);

							ofPushStyle();
							{
								ofNoFill();
								ofSetColor(ofxCvGui::Utils::getBeatingSelectionColor());
								ofDrawCircle(glm::vec3(), 0.1f);
							}
							ofPopStyle();
						}
						ofPopMatrix();
					}
				}

			}
		}
	}
}
#include "MovingHeadToWorld.h"

#include "ofxRulr/Nodes/DMX/MovingHead.h"
#include "ofxRulr/Nodes/DMX/Sharpy.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Utils/Utils.h"

#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/Button.h"

#include "../../../addons/ofxGLM/src/ofxGLM.h"

using namespace ofxCvGui;
using namespace ofxGLM;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark MovingHeadToWorld::Model
				//---------
				MovingHeadToWorld::Model::Model() {

				}

				//---------
				MovingHeadToWorld::Model::Model(const ofVec3f & initialPosition, const ofVec3f & initialRotationEuler) {
					this->initialPosition = initialPosition;
					this->initialRotationEuler = initialRotationEuler;
				}

				//---------
				unsigned int MovingHeadToWorld::Model::getParameterCount() const {
					return 7;
				}

				//---------
				void MovingHeadToWorld::Model::resetParameters() {
					auto parameters = this->getParameters();
					*parameters++ = this->initialPosition[0];
					*parameters++ = this->initialPosition[1];
					*parameters++ = this->initialPosition[2];

					*parameters++ = this->initialRotationEuler[0];
					*parameters++ = this->initialRotationEuler[1];
					*parameters++ = this->initialRotationEuler[2];

					*parameters++ = 0.0; // tiltOffset
				}

				//---------
				double MovingHeadToWorld::Model::getResidual(DataPoint point) const {
					auto pointEvaluated = point;
					try {
						this->evaluate(pointEvaluated);
					}
					catch (...) {
						//invalid position
						return std::numeric_limits<double>::max();
					}

					auto residual = (pointEvaluated.panTilt - point.panTilt).lengthSquared();
					return residual;
				}

				//---------
				void MovingHeadToWorld::Model::evaluate(DataPoint & point) const {
					auto pointInObjectSpace = point.world * this->transform.getInverse();
					point.panTilt = DMX::MovingHead::getPanTiltForTargetInObjectSpace(pointInObjectSpace, this->tiltOffset);
				}

				//---------
				void MovingHeadToWorld::Model::cacheModel() {
					if (this->isReady()) {
						auto parameters = this->getParameters();

						auto position = ofVec3f(*parameters++, *parameters++, *parameters++);
						auto rotationEuler = ofVec3f(*parameters++, *parameters++, *parameters++);
						auto rotationQuat = toOf(glm::quat(toGLM(rotationEuler)));
						this->transform = ofMatrix4x4::newRotationMatrix(rotationQuat) * ofMatrix4x4::newTranslationMatrix(position);

						this->tiltOffset = *parameters++;
					}
				}

				//---------
				const ofMatrix4x4 & MovingHeadToWorld::Model::getTransform() {
					return this->transform;
				}

				//---------
				float MovingHeadToWorld::Model::getTiltOffset() const {
					return this->tiltOffset;
				}

#pragma mark MovingHeadToWorld
				//---------
				MovingHeadToWorld::MovingHeadToWorld() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				void MovingHeadToWorld::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::RigidBody>("Marker");
					this->addInput<DMX::MovingHead>();

					auto view = make_shared<Panels::Base>();
					view->onDraw += [this](ofxCvGui::DrawArguments & args) {
						auto movingHead = this->getInput<DMX::MovingHead>();
						if (movingHead) {
							ofPushStyle();
							{
								auto viewWidth = args.localBounds.width;
								auto viewHeight = args.localBounds.height;

								ofSetLineWidth(1.0f);
								//center line
								ofDrawLine(viewWidth / 2.0f, 0.0f, viewWidth / 2.0f, viewHeight);
								//horizon line
								auto horizonHeight = ofMap(90.0f, 0.0f, movingHead->getMaxTilt(), 0.0f, viewHeight);
								ofDrawLine(0.0f, horizonHeight, viewWidth, horizonHeight);


								auto panTiltToView = [movingHead, viewWidth, viewHeight](const ofVec2f & panTilt) {
									auto drawX = ofMap(panTilt.x, +180.0f, -180.0f, 0.0f, viewWidth);
									auto drawY = ofMap(panTilt.y, 0.0f, movingHead->getMaxTilt(), 0.0f, viewHeight);
									return ofVec2f(drawX, drawY);
								};

								ofFill();
								ofCircle(panTiltToView(movingHead->getPanTilt()), 5.0f);
								ofNoFill();
								ofSetLineWidth(2.0f);
								for (const auto & dataPoint : this->dataPoints) {
									const auto positionInView = panTiltToView(dataPoint.panTilt);
									if (dataPoint.residual != 0) {
										ofDrawBitmapString(ofToString(dataPoint.residual, 3), positionInView);
										ofLine(positionInView, panTiltToView(dataPoint.panTiltEvaluated));
									}
									ofCircle(positionInView, 5.0f);
								}
							}
							ofPopStyle();
						}
					};
					auto viewWeak = weak_ptr<Panels::Base>(view);
					view->onMouse += [this, viewWeak](MouseArguments & args) {
						auto view = viewWeak.lock();
						if (view) {
							//take the mouse press (for drag later)
							args.takeMousePress(view);

							auto movingHead = this->getInput<DMX::MovingHead>();
							if (movingHead) {
								//if dragging
								if (args.isDragging(view)) {
									auto panTilt = movingHead->getPanTilt();
									auto movement = args.movement * (ofGetKeyPressed(OF_KEY_SHIFT) ? 0.01f : 0.2f);
									panTilt.x = ofClamp(panTilt.x - movement.x, -180.0f, +180.0f);
									panTilt.y = ofClamp(panTilt.y + movement.y, 0.0f, movingHead->getMaxTilt());
									movingHead->setPanTilt(panTilt);
								}
							}
						}
					};
					this->view = view;

					this->lastFind = 0.0f;
					this->residual = 0.0f;
				}

				//---------
				string MovingHeadToWorld::getTypeName() const {
					return "Procedure::Calibrate::MovingHeadToWorld";
				}

				//---------
				void MovingHeadToWorld::update() {
					auto movingHead = this->getInput<DMX::MovingHead>();
					if (movingHead) {
						//fade the brightness based on last find time
						float ageOfLastFind = ofGetElapsedTimef() - lastFind;
						movingHead->setBrightness(ofMap(ageOfLastFind, 0, 1.0f, 1.0f, 0.2f, true));
					}
				}

				//---------
				void MovingHeadToWorld::serialize(Json::Value & json) {
					auto & jsonDataPoints = json["dataPoints"];
					for (int i = 0; i < this->dataPoints.size(); i++) {
						jsonDataPoints[i]["world"] << this->dataPoints[i].world;
						jsonDataPoints[i]["panTilt"] << this->dataPoints[i].panTilt;
						jsonDataPoints[i]["residual"] << this->dataPoints[i].residual;
						jsonDataPoints[i]["panTiltEvaluated"] << this->dataPoints[i].panTiltEvaluated;
					}
					json["residual"] << this->residual;
				}

				//---------
				void MovingHeadToWorld::deserialize(const Json::Value & json) {
					this->dataPoints.clear();
					const auto & jsonDataPoints = json["dataPoints"];
					for (const auto & jsonDataPoint : jsonDataPoints) {
						DataPoint dataPoint;
						jsonDataPoint["world"] >> dataPoint.world;
						jsonDataPoint["panTilt"] >> dataPoint.panTilt;
						jsonDataPoint["residual"] >> dataPoint.residual;
						jsonDataPoint["panTiltEvaluated"] >> dataPoint.panTiltEvaluated;
						this->dataPoints.push_back(dataPoint);
					}
					json["residual"] >> this->residual;
				}

				//---------
				void MovingHeadToWorld::populateInspector(ElementGroupPtr inspector) {
					auto addCaptureButton = Widgets::Button::make("Add capture", [this]() {
						bool success = false;
						try {
							this->addCapture();
							success = true;
						}
						RULR_CATCH_ALL_TO_ERROR;

						if (success) {
							Utils::playSuccessSound();
						}
						else {
							Utils::playFailSound();
						}
					}, ' ');
					addCaptureButton->setHeight(100.0f);
					inspector->add(addCaptureButton);
					inspector->add(Widgets::LiveValue<size_t>::make("Data points", [this]() {
						return this->dataPoints.size();
					}));
					inspector->add(Widgets::Button::make("Clear captures", [this]() {
						this->dataPoints.clear();
					}));

					auto calibrateButton = Widgets::Button::make("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT

					}, OF_KEY_RETURN);
					calibrateButton->setHeight(100.0f);
					inspector->add(calibrateButton);

					inspector->add(Widgets::LiveValue<float>::make("Residual", [this]() {
						return this->residual;
					}));
				}

				//---------
				void MovingHeadToWorld::drawWorld() {
					ofMesh lines;
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto movingHeadRotation = movingHead ? movingHead->getRotationQuat() : ofQuaternion();

					for (const auto & dataPoint : this->dataPoints) {
						lines.addVertex(dataPoint.world);
						lines.addColor(ofColor(255, 255));
						auto vector = ofVec3f(0, 0.1f, 0.0f);
						auto rotation = movingHeadRotation * ofQuaternion(dataPoint.panTilt.x, ofVec3f(0, -1, 0)) * ofQuaternion(dataPoint.panTilt.y, ofVec3f(1,0,0));
						lines.addVertex(vector * rotation + dataPoint.world);
						lines.addColor(ofColor(255, 0));
					}
					lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
					lines.draw();
					glPushAttrib(GL_POINT_BIT);
					{
						glPointSize(5.0f);
						glEnable(GL_POINT_SMOOTH);
						lines.drawVertices(); //draw as dots also
					}
					glPopAttrib();
				}

				//---------
				ofxCvGui::PanelPtr MovingHeadToWorld::getView() {
					return this->view;
				}

				//---------
				void MovingHeadToWorld::addCapture() {
					this->throwIfMissingAnyConnection();
					auto movingHead = this->getInput<DMX::MovingHead>();
					auto target = this->getInput<Item::RigidBody>();

					if (target->getTransform().isIdentity()) {
						//presume we didn't get any tracking if it's 0,0
						throw(Exception("Target has no transform, presuming bad tracking."));
					}
					DataPoint dataPoint = {
						target->getPosition(),
						movingHead->getPanTilt(),
						0.0f
					};

					this->dataPoints.push_back(dataPoint);
					this->lastFind = ofGetElapsedTimef();
					Utils::playSuccessSound();
				}

				//---------
				void MovingHeadToWorld::calibrate() {
					this->throwIfMissingAConnection<DMX::MovingHead>();
					auto movingHead = this->getInput<DMX::MovingHead>();

					auto fit = ofxNonLinearFit::Fit<Model>();
					auto model = Model(movingHead->getPosition(), movingHead->getRotationEuler());

					double residual;
					fit.optimise(model, &this->dataPoints, &residual);

					movingHead->setTransform(model.getTransform());
					movingHead->setTiltOffset(model.getTiltOffset());

					this->residual = residual;

					for (auto & dataPoint : this->dataPoints) {
						auto dataPointEvaluated = dataPoint;
						model.evaluate(dataPointEvaluated);
						dataPoint.residual = model.getResidual(dataPoint);
						dataPoint.panTiltEvaluated = dataPointEvaluated.panTilt;
					}

					Utils::playSuccessSound();
				}
			}
		}
	}
}
#include "pch_RulrNodes.h"
#include "RigidBody.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Spacer.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"

#include "ofxGLM.h"

using namespace ofxCvGui;
using namespace ofxCv;
using namespace cv;
using namespace ofxGLM;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
#pragma mark RigidBody
			//---------
			RigidBody::RigidBody() {
				this->onInit += [this]() {
					this->init();
				};
				//RULR_NODE_INIT_LISTENER;
			}

			//---------
			string RigidBody::getTypeName() const {
				return "Item::RigidBody";
			}

			//---------
			void RigidBody::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->translation[0].set("Translation X", 0, -30.0f, 30.0f);
				this->translation[1].set("Translation Y", 0, -30.0f, 30.0f);
				this->translation[2].set("Translation Z", 0, -30.0f, 30.0f);
				this->rotationEuler[0].set("Rotation X", 0, -180.0f, 180.0f);
				this->rotationEuler[1].set("Rotation Y", 0, -180.0f, 180.0f);
				this->rotationEuler[2].set("Rotation Z", 0, -180.0f, 180.0f);
			}

			//---------
			void RigidBody::drawWorld() {
				ofPushMatrix();
				ofMultMatrix(this->getTransform());
				ofDrawAxis(0.3f);
				ofDrawBitmapString(this->getName(), ofVec3f());
				this->onDrawObject.notifyListeners();
				ofPopMatrix();
			}

			//---------
			void RigidBody::serialize(Json::Value & json) {
				auto & jsonTransform = json["transform"];

				auto & jsonTranslation = jsonTransform["translation"];
				for (int i = 0; i < 3; i++){
					jsonTranslation[i] = this->translation[i].get();
				}

				auto & jsonRotationEuler = jsonTransform["rotationEuler"];
				for (int i = 0; i < 3; i++){
					jsonRotationEuler[i] = this->rotationEuler[i].get();
				}

				Utils::Serializable::serialize(json, this->movementSpeed);
			}

			//---------
			void RigidBody::deserialize(const Json::Value & json) {
				auto & jsonTransform = json["transform"];

				auto & jsonTranslation = jsonTransform["translation"];
				for (int i = 0; i < 3; i++){
					this->translation[i] = jsonTranslation[i].asFloat();
				}

				auto & jsonRotationEuler = jsonTransform["rotationEuler"];
				for (int i = 0; i < 3; i++){
					this->rotationEuler[i] = jsonRotationEuler[i].asFloat();
				}

				Utils::Serializable::deserialize(json, this->movementSpeed);
			}

			//---------
			void RigidBody::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				//NOTE : WHEN WE CHANGE THESE FROM SLIDERS, LETS ALSO GET RID OF THE RANGE ON THE PARAMETERS
				// TRANSLATION SHOULDN'T BE BOUND to +/-100

				inspector->add(new Widgets::Title("RigidBody", Widgets::Title::Level::H2));

				inspector->add(make_shared<Widgets::Button>("Clear Transform", [this]() {
					this->clearTransform();
				}));

				for (int i = 0; i < 3; i++) {
					auto slider = new Widgets::Slider(this->translation[i]);
					slider->onValueChange += [this](ofParameter<float> &) {
						this->onTransformChange.notifyListeners();
					};
					inspector->add(slider);

				}
				for (int i = 0; i < 3; i++) {
					auto slider = new Widgets::Slider(this->rotationEuler[i]);
					slider->onValueChange += [this](ofParameter<float> &) {
						this->onTransformChange.notifyListeners();
					};
					inspector->add(slider);
				}

				inspector->add(new Widgets::Button("Export RigidBody matrix...", [this]() {
					try {
						this->exportRigidBodyMatrix();
					}
					RULR_CATCH_ALL_TO_ALERT
				}));

				//WASD controls
				{
					auto element = make_shared<ofxCvGui::Element>();
					auto movementEnabled = make_shared<ofParameter<bool>>("Enabled", false);
					
					//we put it into the scope of the elemnt so that it will be deleted when that scope is deleted
					auto enabledButton = make_shared<Widgets::Toggle>(*movementEnabled);
					enabledButton->addListenersToParent(element);

					element->onDraw += [this, enabledButton, movementEnabled](ofxCvGui::DrawArguments & args) {
						auto drawKey = [](char key, float x, float y) {
							ofPushStyle();
							{
								auto keyIsPressed = ofGetKeyPressed(key) || ofGetKeyPressed(key - ('A' - 'a'));
								if (keyIsPressed) {
									ofFill();
								}
								else {
									ofSetLineWidth(1.0f);
									ofNoFill();
								}
								ofDrawRectangle(x, y, 30, 30);

								if (keyIsPressed) {
									ofSetColor(40);
								}
								else {
									ofSetColor(200);
								}
								ofDrawBitmapString(string(1, key), x + (30 - 8) / 2, y + (30 + 10) / 2);
							}
							ofPopStyle();
						};
						ofPushStyle();
						{
							ofSetColor(150, 200, 150);
							drawKey('W', 50, 10);
							drawKey('A', 10, 50);

							ofSetColor(200, 150, 150);
							drawKey('S', 50, 50);
							drawKey('D', 90, 50);

							ofSetColor(150, 150, 200);
							drawKey('T', 150, 10);
							drawKey('G', 150, 50);
						}
						ofPopStyle();
					};
					element->onKeyboard += [this, enabledButton](ofxCvGui::KeyboardArguments & args) {
						if (args.action == ofxCvGui::KeyboardArguments::Action::Pressed) {
							int axes = 0;
							switch (args.key) {
							case 'w':
							case 'W':
							case 's':
							case 'S':
								axes = 1;
								break;
							case 'a':
							case 'A':
							case 'd':
							case 'D':
								axes = 0;
								break;
							case 't':
							case 'T':
							case 'g':
							case 'G':
								axes = 2;
								break;
							default:
								//no action
								return;
								break;
							};

							float difference = 1.0f;
							switch (args.key)
							{
							case 'w':
							case 'W':
							case 'a':
							case 'A':
							case 'g':
							case 'G':
								difference = -1.0f;
								break;

							case 's':
							case 'S':
							case 'd':
							case 'D':
							case 't':
							case 'T':
								difference = +1.0f;
								break;

							default:
								break;
							}
							this->translation[axes] += difference * this->movementSpeed;
							this->onTransformChange.notifyListeners();
						}
					};
					element->onBoundsChange += [enabledButton](ofxCvGui::BoundsChangeArguments & args) {
						auto bounds = args.localBounds;
						bounds.x = 150 + 30 + 10;
						bounds.width -= bounds.x;
						enabledButton->setBounds(bounds);
					};
					element->setHeight(80.0f);
					inspector->add(element);
					inspector->addSlider(this->movementSpeed);
				}

				inspector->add(make_shared<Widgets::Spacer>());
			}

			//---------
			ofMatrix4x4 RigidBody::getTransform() const {
				//rotation
				auto quat = glm::quat(glm::vec3(this->rotationEuler[0] * DEG_TO_RAD, this->rotationEuler[1] * DEG_TO_RAD, this->rotationEuler[2] * DEG_TO_RAD));
				auto transform = toOf(glm::toMat4(quat));

				//translation
				((ofVec4f*)&transform)[3] = ofVec4f(this->translation[0], this->translation[1], this->translation[2], 1.0f);

				return transform;
			}

			//---------
			ofVec3f RigidBody::getPosition() const {
				ofVec3f position;
				for (int i = 0; i < 3; i++) {
					position[i] = this->translation[i];
				}
				return position;
			}

			//---------
			ofQuaternion RigidBody::getRotationQuat() const {
				return toOf(glm::quat(toGLM(getRotationEuler())));
			}

			//---------
			ofVec3f RigidBody::getRotationEuler() const {
				ofVec3f rotationEuler;
				for (int i = 0; i < 3; i++) {
					rotationEuler[i] = this->rotationEuler[i];
				}
				return rotationEuler;
			}

			//---------
			void RigidBody::setTransform(const ofMatrix4x4 & transform) {
				auto translation = ((ofVec4f*)&transform)[3]; //last row is translation. rip it out;

				//first 3x3 is rotation, rip it out and convert it
				glm::mat3 rotationMatrix;
				for (int i = 0; i < 3; i++) {
					for (int j = 0; j < 3; j++) {
						rotationMatrix[i][j] = transform(i, j); //copy out the 3x3 matrix elements
					}
				}
				auto rotationEuler = glm::eulerAngles(glm::toQuat(rotationMatrix));

				for (int i = 0; i < 3; i++) {
					this->translation[i] = translation[i];
					this->rotationEuler[i] = rotationEuler[i];
				}

				this->onTransformChange.notifyListeners();
			}

			//---------
			void RigidBody::setPosition(const ofVec3f & position) {
				for (int i = 0; i < 3; i++) {
					this->translation[i] = position[i];
				}
				this->onTransformChange.notifyListeners();
			}

			//---------
			void RigidBody::setRotationEuler(const ofVec3f & rotationEuler) {
				for (int i = 0; i < 3; i++) {
					this->rotationEuler[i] = rotationEuler[i];
				}
				this->onTransformChange.notifyListeners();
			}

			//----------
			void RigidBody::setExtrinsics(cv::Mat rotation, cv::Mat translation, bool inverse) {
				auto extrinsicsMatrix = ofxCv::makeMatrix(rotation, translation);
				if (inverse) {
					extrinsicsMatrix = toOf(glm::inverse(toGLM(extrinsicsMatrix)));
					this->setTransform(extrinsicsMatrix);
				}
				else {
					this->setTransform(extrinsicsMatrix);
				}
			}

			//----------
			void RigidBody::clearTransform() {
				for (int i = 0; i < 3; i++) {
					this->translation[i] = 0.0f;
					this->rotationEuler[i] = 0.0f;
				}
				this->onTransformChange.notifyListeners();
			}

			//----------
			void RigidBody::exportRigidBodyMatrix() {
				const auto matrix = this->getTransform();
				auto result = ofSystemSaveDialog(this->getName() + "-World.mat", "Export RigidBody matrix");
				if (result.bSuccess) {
					ofstream fileout(ofToDataPath(result.filePath), ios::binary | ios::out);
					fileout.write((char*)& matrix, sizeof(matrix));
					fileout.close();
				}
			}

#pragma mark Helpers
			/*
			//using glm now

			//---------
			ofVec3f toEuler(const ofQuaternion & rotation) {
				//from http://www.cs.stanford.edu/~acoates/quaternion.h
				ofVec3f euler;
				const static double PI_OVER_2 = PI * 0.5;
				const static double EPSILON = 1e-10;
				double sqw, sqx, sqy, sqz;

				// quick conversion to Euler angles to give tilt to user
				sqw = rotation[3] * rotation[3];
				sqx = rotation[0] * rotation[0];
				sqy = rotation[1] * rotation[1];
				sqz = rotation[2] * rotation[2];

				euler[1] = asin(2.0 * (rotation[3] * rotation[1] - rotation[0] * rotation[2]));
				if (PI_OVER_2 - fabs(euler[1]) > EPSILON) {
					euler[2] = atan2(2.0 * (rotation[0] * rotation[1] + rotation[3] * rotation[2]),
						sqx - sqy - sqz + sqw);
					euler[0] = atan2(2.0 * (rotation[3] * rotation[0] + rotation[1] * rotation[2]),
						sqw - sqx - sqy + sqz);
				}
				else {
					// compute heading from local 'down' vector
					euler[2] = atan2(2 * rotation[1] * rotation[2] - 2 * rotation[0] * rotation[3],
						2 * rotation[0] * rotation[2] + 2 * rotation[1] * rotation[3]);
					euler[0] = 0.0;

					// If facing down, reverse yaw
					if (euler[1] < 0)
						euler[2] = PI - euler[2];
				}
				return euler * RAD_TO_DEG;
			}

			//---------
			ofQuaternion toQuaternion(const ofVec3f & rotationEulerDegrees) {
				//from http://www.cs.stanford.edu/~acoates/quaternion.h
				ofQuaternion rotation;
				float c1 = cos(rotationEulerDegrees[2] * 0.5 * DEG_TO_RAD);
				float c2 = cos(rotationEulerDegrees[1] * 0.5 * DEG_TO_RAD);
				float c3 = cos(rotationEulerDegrees[0] * 0.5 * DEG_TO_RAD);
				float s1 = sin(rotationEulerDegrees[2] * 0.5 * DEG_TO_RAD);
				float s2 = sin(rotationEulerDegrees[1] * 0.5 * DEG_TO_RAD);
				float s3 = sin(rotationEulerDegrees[0] * 0.5 * DEG_TO_RAD);

				rotation[0] = c1*c2*s3 - s1*s2*c3;
				rotation[1] = c1*s2*c3 + s1*c2*s3;
				rotation[2] = s1*c2*c3 - c1*s2*s3;
				rotation[3] = c1*c2*c3 + s1*s2*s3;

				return rotation;
			}
			*/
		}
	}
}
#include "pch_RulrNodes.h"
#include "RigidBody.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Spacer.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"

#include "../../../addons/ofxGLM/src/ofxGLM.h"

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
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->translation[0].set("Translation X", 0, -30.0f, 30.0f);
				this->translation[1].set("Translation Y", 0, -30.0f, 30.0f);
				this->translation[2].set("Translation Z", 0, -30.0f, 30.0f);
				this->rotationEuler[0].set("Rotation X", 0, -360.0f, 360.0f);
				this->rotationEuler[1].set("Rotation Y", 0, -360.0f, 360.0f);
				this->rotationEuler[2].set("Rotation Z", 0, -360.0f, 360.0f);
			}

			//---------
			void RigidBody::drawWorld() {
				ofPushMatrix();
				ofMultMatrix(this->getTransform());
				ofDrawAxis(0.3f);
				ofDrawBitmapString(this->getName(), ofVec3f());
				this->drawObject();
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
			}

			//---------
			void RigidBody::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				//NOTE : WHEN WE CHANGE THESE FROM SLIDERS, LETS ALSO GET RID OF THE RANGE ON THE PARAMETERS
				// TRANSLATION SHOULDN'T BE BOUND to +/-100

				inspector->add(Widgets::Title::make("RigidBody", Widgets::Title::Level::H2));

				inspector->add(make_shared<Widgets::Button>("Clear Transform", [this]() {
					this->clearTransform();
				}));

				for (int i = 0; i < 3; i++) {
					auto slider = Widgets::Slider::make(this->translation[i]);
					slider->onValueChange += [this](ofParameter<float> &) {
						this->onTransformChange.notifyListeners();
					};
					inspector->add(slider);

				}
				for (int i = 0; i < 3; i++) {
					auto slider = Widgets::Slider::make(this->rotationEuler[i]);
					slider->onValueChange += [this](ofParameter<float> &) {
						this->onTransformChange.notifyListeners();
					};
					inspector->add(slider);
				}

				inspector->add(Widgets::Button::make("Export RigidBody matrix...", [this]() {
					try {
						this->exportRigidBodyMatrix();
					}
					RULR_CATCH_ALL_TO_ALERT
				}));

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
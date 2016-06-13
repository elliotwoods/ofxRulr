#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvMin/src/ofxCvMin.h"

#define RULR_RIGIDBODY_DRAW_OBJECT_LISTENER \
	this->onDrawObject += [this]() { \
		this->drawObject(); \
	}

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class RigidBody : public virtual Nodes::Base {
			public:
				RigidBody();
				virtual string getTypeName() const override;
				void init();
				void drawWorld();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);

				ofMatrix4x4 getTransform() const;
				ofVec3f getPosition() const;
				ofQuaternion getRotationQuat() const;
				ofVec3f getRotationEuler() const;

				void setTransform(const ofMatrix4x4 &);
				void setPosition(const ofVec3f &);
				void setRotationEuler(const ofVec3f &);
				void setExtrinsics(cv::Mat rotation, cv::Mat translation, bool inverse = false);
				void clearTransform();

				ofxLiquidEvent<void> onDrawObject;
				ofxLiquidEvent<void> onTransformChange;
			protected:
				void exportRigidBodyMatrix();

				ofParameter<float> translation[3];
				ofParameter<float> rotationEuler[3];
				ofParameter<float> movementSpeed{ "Movement speed [m/s]", 0.1, 0, 10 };
			private:
			};

			ofVec3f toEuler(const ofQuaternion &);
			ofQuaternion toQuaternion(const ofVec3f & degrees);
		}
	}
}
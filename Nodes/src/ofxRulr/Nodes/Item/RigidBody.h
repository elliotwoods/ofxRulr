#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvMin/src/ofxCvMin.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class RigidBody : public Nodes::Base {
			public:
				RigidBody();
				virtual string getTypeName() const override;
				void init();
				void drawWorld() override;
				virtual void drawObject() { }

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::ElementGroupPtr);

				ofMatrix4x4 getTransform() const;
				void setTransform(const ofMatrix4x4 &);
				void setExtrinsics(cv::Mat rotation, cv::Mat translation, bool inverse = false);

				ofxLiquidEvent<void> onTransformChange;
			protected:
				void exportRigidBodyMatrix();

				ofParameter<float> translation[3];
				ofParameter<float> rotationEuler[3];

			private:
			};

			ofVec3f toEuler(const ofQuaternion &);
			ofQuaternion toQuaternion(const ofVec3f & degrees);
		}
	}
}
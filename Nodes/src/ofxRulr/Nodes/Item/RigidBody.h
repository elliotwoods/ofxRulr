#pragma once

#include "ofxRulr/Utils/Constants.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxCvMin/src/ofxCvMin.h"

#define RULR_RIGIDBODY_DRAW_OBJECT_LISTENER \
	this->onDrawObject += [this]() { \
		this->drawObject(); \
	}
#define RULR_RIGIDBODY_DRAW_OBJECT_ADVANCED_LISTENER \
	this->onDrawObjectAdvanced += [this](ofxRulr::DrawWorldAdvancedArgs & args) { \
		this->drawObjectAdvanced(args); \
	}

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class RigidBody : public virtual Nodes::Base {
			public:
				RigidBody();
				virtual string getTypeName() const override;
				void init();
				void drawWorldStage();
				void drawWorldAdvanced(DrawWorldAdvancedArgs &);

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);
				void populateInspector(ofxCvGui::InspectArguments &);

				glm::mat4 getTransform() const;
				glm::vec3 getPosition() const;
				glm::quat getRotationQuat() const;
				glm::vec3 getRotationEuler() const;

				void setTransform(const glm::mat4 &);
				void setPosition(const glm::vec3 &);
				void setRotationEuler(const glm::vec3&);
				void setRotationQuat(const glm::quat&);

				/* Rotation/Translation from object's reference frame to world reference frame (or inverse) */
				void setExtrinsics(cv::Mat rotationVector, cv::Mat translation, bool inverse = false);

				/* Rotation/Translation from object's reference frame to world reference frame (or inverse) */
				void getExtrinsics(cv::Mat & rotationVector, cv::Mat & translation, bool inverse = false);
				
				void clearTransform();
				void applyTransformToNode(ofNode &) const;

				ofxLiquidEvent<void> onDrawObject;
				ofxLiquidEvent<DrawWorldAdvancedArgs> onDrawObjectAdvanced;

				ofxLiquidEvent<void> onTransformChange;
			protected:
				void exportRigidBodyMatrix();

				ofParameter<float> translation[3];
				ofParameter<float> rotationEuler[3];
				ofParameter<float> movementSpeed{ "Movement speed [m/s]", 0.1, 0, 10 };
			private:
			};
		}
	}
}
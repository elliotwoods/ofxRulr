#include "Node.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		class IRigidBody : public Node {
		public:
			IRigidBody();

			ofMatrix4x4 getTransform() const;
			void setTransform(const ofMatrix4x4 &);

			void drawWorld() override;
			virtual void drawObject() = 0;
		protected:
			ofParameter<float> translation[3];
			ofParameter<float> rotationEuler[3];

		private:
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::ElementGroupPtr);
		};
	}
}
#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		class IHasVertices : public virtual Nodes::Base {
		public:
			virtual string getTypeName() const override;
			virtual vector<glm::vec3> getVertices() const;
			ofVec3f getVertexCloseToWorldPosition(const glm::vec3 &) const;
			ofVec3f getVertexCloseToMouse(float maxDistance = 100.0f) const;
			ofVec3f getVertexCloseToMouse(const glm::vec3& mousePosition, float maxDistance = 100.0f) const;
		};
	}
}
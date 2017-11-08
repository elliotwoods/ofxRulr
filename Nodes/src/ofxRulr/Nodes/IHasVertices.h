#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		class IHasVertices : public virtual Nodes::Base {
		public:
			virtual string getTypeName() const override;
			virtual vector<ofVec3f> getVertices() const;
			ofVec3f getVertexCloseToWorldPosition(const ofVec3f &) const;
			ofVec3f getVertexCloseToMouse(float maxDistance = 30.0f) const;
			ofVec3f getVertexCloseToMouse(const ofVec3f & mousePosition, float maxDistance = 30.0f) const;
		};
	}
}
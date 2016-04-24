#pragma once

#include "ofVboMesh.h"

namespace ofxRulr {
	namespace Utils {
		class MeshProvider {
		public:
			MeshProvider();

			void setDimensions(const ofVec2f & dimensions);
			void setDownsampleExp(int exponent);
			void setDirty();

			const ofVec2f & getDimensions() const;
			int getDownsampleExp() const;
			bool isDirty() const;

			const ofVboMesh & getMesh();

		protected:
			void rebuildMesh();

			ofVboMesh mesh;

			ofVec2f dimensions;
			int downsampleExp;
			bool dirty;
		};
	}
}
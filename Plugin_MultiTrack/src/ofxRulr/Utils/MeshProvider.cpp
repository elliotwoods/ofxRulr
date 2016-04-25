#include "pch_MultiTrack.h"
#include "MeshProvider.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		MeshProvider::MeshProvider()
			: dimensions(0, 0)
			, downsampleExp(0)
			, dirty(true) {}

		//----------
		void MeshProvider::setDimensions(const ofVec2f & dimensions) {
			if (this->dimensions == dimensions) return;

			this->dimensions = dimensions;
			this->setDirty();
		}

		//----------
		void MeshProvider::setDownsampleExp(int downsampleExp) {
			if (this->downsampleExp == downsampleExp) return;

			this->downsampleExp = downsampleExp;
			this->setDirty();
		}

		//----------
		void MeshProvider::setDirty() {
			this->dirty = true;
		}

		//----------
		const ofVec2f & MeshProvider::getDimensions() const {
			return this->dimensions;
		}

		//----------
		int MeshProvider::getDownsampleExp() const {
			return this->downsampleExp;
		}

		//----------
		bool MeshProvider::isDirty() const {
			return this->dirty;
		}

		//----------
		const ofVboMesh & MeshProvider::getMesh() {
			if (this->dirty) {
				this->rebuildMesh();
			}

			return this->mesh;
		}

		//----------
		void MeshProvider::rebuildMesh() {
			float stepSize = pow(2, this->downsampleExp);
			ofVec2f numSteps = ofVec2f(ceil(this->dimensions.x / stepSize), ceil(this->dimensions.y / stepSize));

			auto width = numSteps.x * stepSize;
			auto height = numSteps.y * stepSize;

			//Indices
			//{
			//	this->mesh.clearIndices();

			//	int x = 0;
			//	int y = 0;
			//	int width = stepAmt.x * downsampleAmt;
			//	int height = stepAmt.y * downsampleAmt;

			//	for (float yStep = 0; yStep < height - stepAmt.y; yStep += stepAmt.y) {
			//		for (float xStep = 0; xStep < width - stepAmt.x; xStep += stepAmt.x) {
			//			ofIndexType a, b, c;

			//			a = x + y * stepAmt.x;
			//			b = (x + 1) + y*stepAmt.x;
			//			c = x + (y + 1)*stepAmt.x;
			//			mesh.addIndex(a);
			//			mesh.addIndex(b);
			//			mesh.addIndex(c);

			//			a = (x + 1) + (y + 1)*stepAmt.x;
			//			b = x + (y + 1)*stepAmt.x;
			//			c = (x + 1) + (y)*stepAmt.x;
			//			mesh.addIndex(a);
			//			mesh.addIndex(b);
			//			mesh.addIndex(c);

			//			x++;
			//		}

			//		y++;
			//		x = 0;
			//	}
			//}

			//Vertices and texture coordinates
			{
				this->mesh.clearVertices();
				this->mesh.clearTexCoords();
				for (float y = 0; y < height; y += stepSize) {
					for (float x = 0; x < width; x += stepSize) {
						this->mesh.addVertex(ofVec3f(x, y, 0));
						this->mesh.addTexCoord(ofVec2f(x, y));
					}
				}
			}

			//if (addColors) {
			//	mesh.clearColors();
			//	for (float y = 0; y < depthImageSize.height; y += simplify.y) {
			//		for (float x = 0; x < depthImageSize.width; x += simplify.x) {
			//			mesh.addColor(ofFloatColor(1.0, 1.0, 1.0, 1.0));
			//		}
			//	}
			//}

			this->dirty = false;
		}
	}
}
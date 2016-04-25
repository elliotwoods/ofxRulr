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
			ofVec2f downsampledResolution = ofVec2f(ceil(this->dimensions.x / stepSize), ceil(this->dimensions.y / stepSize));

			auto width = downsampledResolution.x * stepSize;
			auto height = downsampledResolution.y * stepSize;

 			//Indices
			{
				this->mesh.clearIndices();

				int x = 0;
				int y = 0;

				auto stride = downsampledResolution.x;

				for (int y = 0; y < downsampledResolution.y - 1; y++) {
					for (int x = 0; x < downsampledResolution.x - 1; x++) {
						ofIndexType TL = x + y * stride;
						ofIndexType BL = x + (y + 1) * stride;
						ofIndexType TR = (x + 1) + y * stride;
						ofIndexType BR = (x + 1) + (y + 1) * stride;

						// top left triangle
						mesh.addIndices({ TL, BL, TR });

						// bottom right triangle
						mesh.addIndices({ TR, BL, BR });
					}
				}
			}

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

			//cout << "Built mesh with " << this->mesh.getNumVertices() << " vertices" << endl;

			this->dirty = false;
		}
	}
}
#include "pch_RulrNodes.h"
#include "IHasVertices.h"

namespace ofxRulr {
	namespace Nodes {
		//----------
		std::string IHasVertices::getTypeName() const {
			return "IHasVertices";
		}

		//----------
		std::vector<glm::vec3> IHasVertices::getVertices() const {
			return vector<glm::vec3>();
		}

		//----------
		ofVec3f IHasVertices::getVertexCloseToWorldPosition(const glm::vec3 & pickPosition) const {
			ofVec3f closestVertex = pickPosition; // default value
			float minDistance = numeric_limits<float>::max();

			const auto vertices = this->getVertices();
			for (const auto & vertex : vertices) {
				auto distance = glm::distance(vertex, pickPosition);
				if (minDistance > distance) {
					closestVertex = vertex;
					minDistance = distance;
				}
			}

			return closestVertex;
		}

		//----------
		ofVec3f IHasVertices::getVertexCloseToMouse(float maxDistance /*= 30.0f*/) const {
			return getVertexCloseToMouse(ofVec3f(ofGetMouseX(), ofGetMouseY(), 0.0f), maxDistance);
		}

		//----------
		ofVec3f IHasVertices::getVertexCloseToMouse(const glm::vec3 & mousePosition, float maxDistance) const {
			float minDistance = std::numeric_limits<float>::max();
			ofVec3f closestVertex;
			bool foundVertex = false;

			auto worldStage = ofxRulr::Graph::World::X().getWorldStage();
			auto & camera = worldStage->getCamera();
			auto worldStagePanelBounds = worldStage->getPanel()->getBounds();

			const auto vertices = this->getVertices();

			for(const auto & vertex : vertices) {
				auto vertexScreen = camera.worldToScreen(vertex, worldStagePanelBounds);
				vertexScreen.z = 0.0f;

				auto distance = glm::distance(vertexScreen, mousePosition);
				if (distance > maxDistance) {
					continue;
				}
				if (minDistance > distance) {
					closestVertex = vertex;
					minDistance = distance;
					foundVertex = true;
				}
			}

			if (foundVertex) {
				return closestVertex;
			}
			throw(ofxRulr::Exception("No vertex could be found"));
		}

	}
}
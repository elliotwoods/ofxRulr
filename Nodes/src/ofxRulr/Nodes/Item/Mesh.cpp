#include "pch_RulrNodes.h"
#include "Mesh.h"

#include <ofxCvGui/Widgets/Button.h>
#include <ofxCvGui/Widgets/LiveValue.h>
#include <ofxCvGui/Widgets/Toggle.h>
#include <ofxCvGui/Widgets/Title.h>
#include <ofxCvGui/Widgets/Slider.h>

#include "ofxAssimpModelLoader.h"

#include "ofSystemUtils.h"

#include "ofxRulr/Graph/World.h"

using namespace ofxCvGui;
using namespace ofxCvGui::Widgets;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Mesh::Mesh() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Mesh::getTypeName() const {
				return "Item::Mesh";
			}

			//----------
			void Mesh::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->modelLoader = make_unique<ofxAssimpModelLoader>();
				this->onDrawObject += [this]() {
					static ofLight light;
					ofPushStyle();
					{
						light.enable();
						ofSetColor(this->parameters.drawStyle.color.get());
						this->drawObject();
						light.disable();
					}
					ofPopStyle();
				};
			}

			//----------
			void Mesh::update() {

			}

			//----------
			void Mesh::drawObject() {
				if (!this->modelLoader) {
					return;
				}

				ofPushMatrix();
				{
					ofMultMatrix(this->getMeshTransform());
				}

				if (this->parameters.drawStyle.vertices) {
					Utils::Graphics::pushPointSize(3.0f);
					{
						this->modelLoader->drawVertices();
					}
					Utils::Graphics::popPointSize();
				}
				if (this->parameters.drawStyle.wireframe) {
					this->modelLoader->drawWireframe();
				}
				if (this->parameters.drawStyle.faces) {
					this->modelLoader->drawFaces();
				}
				ofPopMatrix();
			}

			//----------
			void Mesh::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->filename);
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Mesh::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->filename);

				if (this->filename.get().empty()) {
					this->modelLoader->clear();
				}
				else {
					this->modelLoader->loadModel(this->filename.get());
					modelLoader->calculateDimensions();
				}

				Utils::Serializable::deserialize(json, this->parameters);
			}


			ofVec4f axesToVector(const Mesh::Axes & axis) {
				switch (axis.get()) {
				case Mesh::Axes::NegX:
					return ofVec4f(-1, 0, 0, 0);
				case Mesh::Axes::PosX:
					return ofVec4f(+1, 0, 0, 0);
				case Mesh::Axes::NegY:
					return ofVec4f(0, -1, 0, 0);
				case Mesh::Axes::PosY:
					return ofVec4f(0, +1, 0, 0);
				case Mesh::Axes::NegZ:
					return ofVec4f(0, 0, -1, 0);
				case Mesh::Axes::PosZ:
					return ofVec4f(0, 0, +1, 0);
				default:
					return ofVec4f();
				}
			}
			//----------
			ofMatrix4x4 Mesh::getMeshTransform() const {
				ofMatrix4x4 transform;

				auto row = (ofVec4f*)transform.getPtr();
				*row++ = axesToVector(this->parameters.transform.theirXIsOur);
				*row++ = axesToVector(this->parameters.transform.theirYIsOur);
				*row++ = axesToVector(this->parameters.transform.theirZIsOur);

				transform.postMultScale(ofVec3f(this->parameters.transform.scale));

				return transform;
			}

			//----------
			ofVec3f Mesh::getVertexCloseToWorldPosition(const ofVec3f & pickPosition) const {
				ofVec3f closestVertex = pickPosition; // default value

				if (this->modelLoader) {
					ofMatrix4x4 transform = this->getMeshTransform();
					const auto meshCount = this->modelLoader->getMeshCount();

					float minDistance = numeric_limits<float>::max();

					for (size_t i = 0; i < meshCount; i++) {
						const auto & meshHelper = this->modelLoader->getMeshHelper(i);
						const auto & mesh = this->modelLoader->getMesh(i);
						auto meshTransform = transform * meshHelper.matrix;
						for (const auto & vertex : mesh.getVertices()) {
							auto vertexWorld = vertex * transform;
							auto distance = vertexWorld.distance(pickPosition);
							if (minDistance > distance) {
								closestVertex = vertexWorld;
								minDistance = distance;
							}
						}
					}
				}

				return closestVertex;
			}

			//----------
			ofVec3f Mesh::getVertexCloseToMouse(float maxDistance /*= 30.0f*/) const {
				return getVertexCloseToMouse(ofVec3f(ofGetMouseX(), ofGetMouseY(), 0.0f), maxDistance);
			}

			//----------
			ofVec3f Mesh::getVertexCloseToMouse(const ofVec3f & mousePosition, float maxDistance) const {
				if (this->modelLoader) {
					const auto meshCount = this->modelLoader->getMeshCount();
					float minDistance = std::numeric_limits<float>::max();
					ofVec3f closestVertex;
					bool foundVertex = false;

					auto summary = ofxRulr::Graph::World::X().getSummary();
					auto & camera = summary->getCamera();
					auto summaryBounds = summary->getPanel()->getBounds();

					for (size_t i = 0; i < meshCount; i++) {
						const auto & meshHelper = this->modelLoader->getMeshHelper(i);
						auto meshTransform = meshHelper.matrix * this->modelLoader->getModelMatrix() * this->getMeshTransform() * RigidBody::getTransform();
						for (size_t iVertex = 0; iVertex < meshHelper.mesh->mNumVertices; iVertex++) {
							const auto & vertexObject = *(ofVec3f*)&meshHelper.mesh->mVertices[iVertex];
							auto vertexWorld = vertexObject * meshTransform;

							auto vertexScreen = camera.worldToScreen(vertexWorld, summaryBounds);
							vertexScreen.z = 0.0f;

							auto distance = vertexScreen.distance(mousePosition);
							if (distance > maxDistance) {
								continue;
							}
							if (minDistance > distance) {
								closestVertex = vertexWorld;
								minDistance = distance;
								foundVertex = true;
							}
						}
					}

					if (foundVertex) {
						return closestVertex;
					}
				}
				throw(ofxRulr::Exception("No vertex could be found"));
			}

			//----------
			void Mesh::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				auto loadButton = make_shared<ofxCvGui::Widgets::Button>("Load model...", [this]() {
					auto result = ofSystemLoadDialog("Load model using Assimp");
					if (result.bSuccess) {
						this->modelLoader->loadModel(result.filePath);
						this->modelLoader->calculateDimensions();
						this->filename.set(result.filePath);
					}
				});
				inspector->add(loadButton);

				inspector->add(make_shared<LiveValue<string>>("Filename", [this]() {
					return this->filename.get();
				}));
				auto clearModelButton = make_shared <ofxCvGui::Widgets::Button>("Clear model", [this]() {
					this->modelLoader->clear();
					this->filename.set("");
				});
				//we put the listener on the loadButton since we'll be disabling clearModelButton
				//also it stops there being a circular reference where loadButton owns a listener stack which owns a lambda which owns loadButton
				loadButton->onUpdate += [this, clearModelButton](ofxCvGui::UpdateArguments &) {
					clearModelButton->setEnabled(!this->filename.get().empty());
				};
				inspector->add(clearModelButton);

				inspector->addLiveValue<size_t>("Mesh count", [this]() {
					if (this->modelLoader) {
						return this->modelLoader->getNumMeshes();
					}
					else {
						return 0;
					}
				});
				inspector->addLiveValue<string>("Vertex count", [this]() {
					if (this->modelLoader) {
						auto meshCount = this->modelLoader->getNumMeshes();
						vector<size_t> vertexCounts;
						for (int i = 0; i < meshCount; i++) {
							vertexCounts.push_back(this->modelLoader->getMesh(i).getNumVertices());
						}
						size_t totalCount = 0;
						for (const auto & vertexCount : vertexCounts) {
							totalCount += vertexCount;
						}

						stringstream message;
						message << totalCount << " [";
						bool first = true;
						for (const auto & vertexCount : vertexCounts) {
							if (!first) {
								message << ", ";
							}
							else {
								first = false;
							}
							message << vertexCount;
						}
						message << "]";
						return message.str();
					}
					else {
						return string();
					}
				});
				inspector->addLiveValue<ofVec3f>("Bounds min", [this]() {
					if (this->modelLoader) {
						return this->modelLoader->getSceneMin();
					}
					else {
						return ofVec3f();
					}
				});
				inspector->addLiveValue<ofVec3f>("Bounds max", [this]() {
					if (this->modelLoader) {
						return this->modelLoader->getSceneMax();
					}
					else {
						return ofVec3f();
					}
				});

				inspector->addParameterGroup(this->parameters);
			}
		}
	}
}
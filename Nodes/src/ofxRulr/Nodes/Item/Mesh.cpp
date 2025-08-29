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
#include "ofxRulr/Nodes/Render/Style.h"

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
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_ADVANCED_LISTENER;

				this->addInput<Render::Style>();

				//HACK : This is more common, esp coming from PhotoScan data
				this->parameters.transform.theirXIsOur = Axes::NegX;
				this->parameters.transform.theirYIsOur = Axes::NegY;
			}

			//----------
			void Mesh::update() {
				if (this->meshDirty) {
					this->loadMesh();
				}
				if (this->textureDirty) {
					this->loadTexture();
				}
				if (this->modelLoader) {
					switch (this->parameters.drawStyle.cull.get().get()) {
					case Cull::None:
						this->modelLoader->disableCulling();
						break;
					case Cull::CW:
						this->modelLoader->enableCulling(GL_CW);
						break;
					case Cull::CCW:
						this->modelLoader->enableCulling(GL_CCW);
						break;
					}
				}

				if (this->meshAnalysisCache.needsRecalculate) {
					if (this->modelLoader) {
						// vertex count
						{
							auto meshCount = this->modelLoader->getNumMeshes();
							vector<size_t> vertexCounts;
							for (uint32_t i = 0; i < meshCount; i++) {
								vertexCounts.push_back(this->modelLoader->getMesh(i).getNumVertices());
							}
							size_t totalCount = 0;
							for (const auto& vertexCount : vertexCounts) {
								totalCount += vertexCount;
							}

							stringstream message;
							message << totalCount << " [";
							bool first = true;
							for (const auto& vertexCount : vertexCounts) {
								if (!first) {
									message << ", ";
								}
								else {
									first = false;
								}
								message << vertexCount;
							}
							message << "]";
							this->meshAnalysisCache.vertexCount = message.str();
						}

						this->meshAnalysisCache.numMeshes = this->modelLoader->getNumMeshes();
						this->meshAnalysisCache.sceneMin = this->modelLoader->getSceneMin();
						this->meshAnalysisCache.sceneMax = this->modelLoader->getSceneMax();
					}
					else {
						this->meshAnalysisCache.vertexCount = "";
						this->meshAnalysisCache.sceneMin = { 0, 0, 0 };
						this->meshAnalysisCache.sceneMax = { 0, 0, 0 };
						this->meshAnalysisCache.numMeshes = 0;
					}

					this->meshAnalysisCache.needsRecalculate = false;
				}
			}

			//----------
			void Mesh::drawObject() {
				if (!this->modelLoader) {
					return;
				}

				auto style = this->getInput<Render::Style>();

				if (style) {
					style->begin();
				}

				auto & texture = this->texture.getTexture();
				if (texture.isAllocated()) {
					texture.bind();
				}

				ofPushMatrix();
				{
					ofMultMatrix(this->getMeshTransform());

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
				}
				ofPopMatrix();

				if (texture.isAllocated()) {
					texture.unbind();
				}

				if (style) {
					style->end();
				}
			}

			//----------
			void Mesh::drawObjectAdvanced(DrawWorldAdvancedArgs & args) {

				if (!this->modelLoader) {
					return;
				}

				auto style = this->getInput<Render::Style>();

				if (args.enableStyle) {
					if (style) {
						style->begin();
					}
				}

				if (args.enableTextures) {
					auto& texture = this->texture.getTexture();
					if (texture.isAllocated()) {
						texture.bind();
					}
				}

				ofPushMatrix();
				{
					ofMultMatrix(this->getMeshTransform());

					args.enableMaterials ? modelLoader->enableMaterials() : modelLoader->disableMaterials();
					args.enableColors ? modelLoader->enableColors() : modelLoader->disableColors();
					args.enableTextures ? modelLoader->enableTextures() : modelLoader->disableTextures();
					args.enableNormals ? modelLoader->enableNormals() : modelLoader->disableNormals();

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
				}
				ofPopMatrix();

				if (args.enableTextures) {
					if (texture.isAllocated()) {
						texture.unbind();
					}
				}

				if (args.enableStyle) {
					if (style) {
						style->end();
					}
				}
			}

			//----------
			void Mesh::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->meshFilename);
				Utils::serialize(json, this->textureFilename);
				Utils::serialize(json, this->parameters);
			}

			//----------
			void Mesh::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->textureFilename);
				Utils::deserialize(json, this->meshFilename);
				Utils::deserialize(json, this->parameters);

				this->meshDirty = true;
				this->textureDirty = true;
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

			glm::vec3 operator*(const glm::mat4& m, const glm::vec3& v)
			{
				glm::vec4 v4{ v.x, v.y, v.z, 1.0f };
				v4 = m * v4;
				v4 /= v4.w;
				return { v4.x, v4.y, v4.z };
			}

			//----------
			vector<glm::vec3> Mesh::getVertices() const {
				vector<glm::vec3> vertices;

				if (this->modelLoader) {
					glm::mat4 transform = this->getMeshTransform();
					const auto meshCount = this->modelLoader->getMeshCount();

					for (size_t i = 0; i < meshCount; i++) {
						const auto & meshHelper = this->modelLoader->getMeshHelper(i);
						const auto & mesh = this->modelLoader->getMesh(i);
						auto meshTransform = transform * (glm::mat4) meshHelper.matrix;
						for (const auto & vertex : mesh.getVertices()) {
							vertices.push_back(transform * vertex);
						}
					}
				}

				return vertices;
			}


			//----------
			void Mesh::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				{
					inspector->addTitle("Mesh", ofxCvGui::Widgets::Title::H3);

					auto loadButton = make_shared<ofxCvGui::Widgets::Button>("Load...", [this]() {
						auto result = ofSystemLoadDialog("Load model using Assimp");
						if (result.bSuccess) {
							this->meshFilename.set(result.filePath);
							this->meshDirty = true;
						}
					});
					inspector->add(loadButton);

					inspector->add(make_shared<LiveValue<string>>("Filename", [this]() {
						return this->meshFilename.get();
					}));
					auto clearModelButton = make_shared <ofxCvGui::Widgets::Button>("Clear", [this]() {
						this->meshFilename.set("");
						this->meshDirty = true;
					});
					//we put the listener on the loadButton since we'll be disabling clearModelButton
					//also it stops there being a circular reference where loadButton owns a listener stack which owns a lambda which owns loadButton
					loadButton->onUpdate += [this, clearModelButton](ofxCvGui::UpdateArguments &) {
						clearModelButton->setEnabled(!this->meshFilename.get().empty());
					};
					inspector->add(clearModelButton);
				}
				{
					inspector->addTitle("Texture", ofxCvGui::Widgets::Title::H3);

					auto loadButton = make_shared<ofxCvGui::Widgets::Button>("Load...", [this]() {
						auto result = ofSystemLoadDialog("Load texture");
						if (result.bSuccess) {
							this->textureFilename.set(result.filePath);
							this->textureDirty = true;
						}
					});
					inspector->add(loadButton);

					inspector->add(make_shared<LiveValue<string>>("Filename", [this]() {
						return this->textureFilename.get();
					}));
					auto clearButton = make_shared <ofxCvGui::Widgets::Button>("Clear", [this]() {
						this->textureFilename.set("");
						this->textureDirty = true;
					});
					//we put the listener on the loadButton since we'll be disabling clearModelButton
					//also it stops there being a circular reference where loadButton owns a listener stack which owns a lambda which owns loadButton
					loadButton->onUpdate += [this, clearButton](ofxCvGui::UpdateArguments &) {
						clearButton->setEnabled(!this->textureFilename.get().empty());
					};
					inspector->add(clearButton);
				}
				
				inspector->addLiveValue<uint32_t>("Mesh count", [this]() {
					return this->meshAnalysisCache.numMeshes;
				});
				inspector->addLiveValue<string>("Vertex count", [this]() {
					return this->meshAnalysisCache.vertexCount;
				});
				inspector->addLiveValue<glm::vec3>("Bounds min", [this]() {
					return this->meshAnalysisCache.sceneMin;
				});
				inspector->addLiveValue<glm::vec3>("Bounds max", [this]() {
					return this->meshAnalysisCache.sceneMax;
				});

				inspector->addParameterGroup(this->parameters);
			}

			//----------
			void Mesh::loadMesh() {
				if (this->meshFilename.get().empty()) {
					this->modelLoader.reset();
				}
				else {
					this->modelLoader = make_unique<ofxAssimpModelLoader>();
					if (!this->modelLoader->load(this->meshFilename.get(), ofxAssimpModelLoader::OPTIMIZE_NONE)) {
						this->modelLoader.reset();
					}
				}

				if (this->modelLoader) {
					this->modelLoader->calculateDimensions();
					this->modelLoader->setScaleNormalization(false);
				}
				
				this->meshDirty = false;
			}

			//----------
			void Mesh::loadTexture() {
				if (this->textureFilename.get().empty()) {
					this->texture.clear();
				}
				else {
					this->texture.load(this->textureFilename.get());
				}
				this->textureDirty = false;
			}
		}
	}
}
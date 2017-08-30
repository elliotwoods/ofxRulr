#include "pch_RulrNodes.h"
#include "Model.h"

#include <ofxCvGui/Widgets/Button.h>
#include <ofxCvGui/Widgets/LiveValue.h>
#include <ofxCvGui/Widgets/Toggle.h>
#include <ofxCvGui/Widgets/Title.h>
#include <ofxCvGui/Widgets/Slider.h>

#include "ofxAssimpModelLoader.h"

#include "ofSystemUtils.h"

using namespace ofxCvGui;
using namespace ofxCvGui::Widgets;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Model::Model() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Model::getTypeName() const {
				return "Item::Model";
			}

			//----------
			void Model::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->modelLoader = make_unique<ofxAssimpModelLoader>();
			}

			//----------
			void Model::update() {

			}

			//----------
			void Model::drawWorld() {
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
			void Model::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->filename);
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Model::deserialize(const Json::Value & json) {
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


			ofVec4f axesToVector(const Model::Axes & axis) {
				switch (axis.get()) {
				case Model::Axes::NegX:
					return ofVec4f(-1, 0, 0, 0);
				case Model::Axes::PosX:
					return ofVec4f(+1, 0, 0, 0);
				case Model::Axes::NegY:
					return ofVec4f(0, -1, 0, 0);
				case Model::Axes::PosY:
					return ofVec4f(0, +1, 0, 0);
				case Model::Axes::NegZ:
					return ofVec4f(0, 0, -1, 0);
				case Model::Axes::PosZ:
					return ofVec4f(0, 0, +1, 0);
				default:
					return ofVec4f();
				}
			}
			//----------
			ofMatrix4x4 Model::getMeshTransform() const {
				ofMatrix4x4 transform;
				auto row = (ofVec4f*)transform.getPtr();
				*row++ = axesToVector(this->parameters.transform.theirXIsOur);
				*row++ = axesToVector(this->parameters.transform.theirYIsOur);
				*row++ = axesToVector(this->parameters.transform.theirZIsOur);

				transform.postMultScale(ofVec3f(this->parameters.transform.scale));
				return transform;
			}

			//----------
			void Model::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
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
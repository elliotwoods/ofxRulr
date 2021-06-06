#include "pch_RulrCore.h"
#include "WorldStage.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Graph {
		//----------
		WorldStage::WorldStage() :
		world() {
			RULR_NODE_INIT_LISTENER;
			this->world = nullptr;
		}

		//----------
		string WorldStage::getTypeName() const {
			return "WorldStage";
		}

		//----------
		void WorldStage::init() {
			RULR_NODE_UPDATE_LISTENER;
			RULR_NODE_INSPECTOR_LISTENER;
			RULR_NODE_SERIALIZATION_LISTENERS;

			this->view = MAKE(ofxCvGui::Panels::WorldManaged);
			this->view->onDraw.addListener([this](ofxCvGui::DrawArguments &) {
				ofBackgroundGradient(40, 0);
			}, this, -1);
			this->view->onDrawWorld += [this](ofCamera &) {
				if (this->world) {
					auto & world = *this->world;
					for (const auto node : world) {
						const auto & whenDrawOnWorldStage = node->getWhenDrawOnWorldStage();
						switch (whenDrawOnWorldStage) {
						case WhenDrawOnWorldStage::Selected:
							if (node->isBeingInspected()) {
								node->drawWorldStage();
							}
							break;
						case WhenDrawOnWorldStage::Always:
							node->drawWorldStage();
							break;
						case WhenDrawOnWorldStage::Never:
						default:
							break;
						}
					}
				}
			};
			auto & camera = this->view->getCamera();
			camera.setNearClip(0.01f);
			camera.setFarClip(10000.0f);
			this->camera = &camera;
		}

		//----------
		void WorldStage::setWorld(const Utils::Set<Nodes::Base> & world) {
			this->world = &world;
		}

		//----------
		void WorldStage::update() {

		}

		//----------
		ofxCvGui::PanelPtr WorldStage::getPanel() {
			return this->view;
		}

		//----------
		glm::vec3 WorldStage::getCursorWorld(bool forceUpdate) const {
			auto camera = static_cast<ofxGrabCam *>(this->camera);
			if (forceUpdate) {
				ofMouseEventArgs mouseArgs(ofMouseEventArgs::Type::Moved, ofGetMouseX(), ofGetMouseY());
				camera->mouseMoved(mouseArgs);
				camera->updateCursorWorld();
			}
			return camera->getCursorWorld();
		}

		//----------
		ofxGrabCam & WorldStage::getCamera() {
			return * (ofxGrabCam*) this->camera;
		}

		//----------
		void WorldStage::serialize(nlohmann::json & json) {
			Utils::serialize(json, "view", this->view->parameters);

			auto & camera = this->view->getCamera();
			auto & cameraJson = json["Camera"];
			cameraJson["position"] << camera.getPosition();
			cameraJson["orientation"] << camera.getOrientationQuat();
		}

		//----------
		void WorldStage::deserialize(const nlohmann::json & json) {
			Utils::deserialize(json, "view", this->view->parameters);

			auto & camera = this->view->getCamera();
			if (json.contains("Camera")) {
				auto & cameraJson = json["Camera"];

				{
					glm::quat orientation;
					cameraJson["orientation"] >> orientation;
					camera.setOrientation(orientation);
				}
				
				{
					glm::vec3 position;
					cameraJson["position"] >> position;
					camera.setPosition(position);
				}
			}
			else {
				camera.setPosition(this->view->parameters.grid.roomMin.get() * glm::vec3(0.0f, 1.0f, 1.0f));
				camera.lookAt(this->view->parameters.grid.roomMax.get() * glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0, -1, 0));
				camera.move(glm::vec3(0, 0, 0)); // nudge camera to update
			}
		}

		//----------
		void WorldStage::populateInspector(ofxCvGui::InspectArguments& inspectArguments) {
			auto inspector = inspectArguments.inspector;

			inspector->addParameterGroup(this->view->parameters);
			inspector->addLiveValue<glm::vec3>("Cursor position", [this]() {
				return this->getCamera().getCursorWorld();
				});
		}
	}
}
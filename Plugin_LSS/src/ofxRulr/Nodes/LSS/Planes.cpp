#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
#pragma mark Plane
			//----------
			Planes::Plane::Plane() {
				RULR_NODE_SERIALIZATION_LISTENERS;
			}

			//----------
			string Planes::Plane::getDisplayString() const {
				stringstream ss;
				ss << this->parameters.center << endl;
				ss << this->parameters.normal << endl;
				return ss.str();
			}

			//----------
			void Planes::Plane::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, parameters);
			}

			//----------
			void Planes::Plane::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, parameters);
			}

			//----------
			ofxRay::Plane Planes::Plane::getPlane() {
				ofxRay::Plane plane(this->parameters.center
					, this->parameters.normal
					, this->parameters.up
					, this->parameters.scale);
				plane.color = this->color;
				plane.setInfinite(false);

				if (this->parameters.mask) {
					plane.color = ofColor(30, 0, 0, 0);
				}

				return plane;
			}

			//----------
			ofxCvGui::ElementPtr Planes::Plane::getDataDisplay() {
				auto element = ofxCvGui::Panels::makeWidgets();
				element->addParameterGroup(this->parameters);
				element->arrange();
				
				//element->setHeight(element->getLength()); <--doesn't work
				element->setHeight(40 * 5);

				return element;
			}


#pragma mark Planes
			//----------
			Planes::Planes() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Planes::getTypeName() const {
				return "LSS::Planes";
			}

			//----------
			template<typename T>
			T askData(string name) {
				T data;
				auto response = ofSystemTextBoxDialog(name);
				if (!response.empty()) {
					stringstream inputTextStream(response);
					inputTextStream >> data;
				}
				return data;
			}

			void Planes::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;

				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					panel->addButton("Add...", [this]() {
						auto position = askData<ofVec3f>("Position");
						auto normal = askData<ofVec3f>("Normal");
						auto up = askData<ofVec3f>("Up");

						auto plane = make_shared<Plane>();
						plane->parameters.center = position;
						plane->parameters.normal = normal;
						plane->parameters.up = up;

						this->planes.add(plane);
					});

					this->planes.populateWidgets(panel);

					this->panel = panel;
				}
			}

			//----------
			ofxCvGui::PanelPtr Planes::getPanel() {
				return this->panel;
			}

			//----------
			void Planes::drawWorldStage() {
				auto planes = this->planes.getSelection();
				for (auto plane : planes) {
					plane->getPlane().draw();
				}
			}

			//----------
			void Planes::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;
			}

			//----------
			void Planes::serialize(Json::Value & json) {
				this->planes.serialize(json);
			}

			//----------
			void Planes::deserialize(const Json::Value & json) {
				this->planes.deserialize(json);
			}

			//----------
			vector<shared_ptr<Planes::Plane>> Planes::getPlanes() const {
				return this->planes.getSelection();
			}
		}
	}
}
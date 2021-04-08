#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
#pragma mark Heliostats
				//----------
				Heliostats::Heliostats() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Heliostats::getTypeName() const {
					return "Halo::Heliostats";
				}

				//----------
				void Heliostats::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

// 					{
// 						this->panel = make_shared<ofxCvGui::Panels::Widgets>();
// 						this->panel->addButton("Add...", [this]() {
// 							auto nameString = ofSystemTextBoxDialog("Name");
// 							if (!nameString.empty()) {
// 
// 								auto heliostat = make_shared<Heliostat>();
// 								heliostat->name = nameString;
// 								this->heliostats.add(heliostat);
// 							}
// 						})->setHeight(100.0f);
// 					}
				}

				//----------
				void Heliostats::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					this->heliostats.populateWidgets(inspector);
				}

				//----------
				void Heliostats::serialize(nlohmann::json & json) {
					this->heliostats.serialize(json["heliostats"]);
				}

				//----------
				void Heliostats::deserialize(const nlohmann::json & json) {
					this->heliostats.deserialize(json["heliostats"]);
				}

				//----------
				ofxCvGui::PanelPtr Heliostats::getPanel() {
					return this->panel;
				}

				//----------
				void Heliostats::drawWorldStage() {
					auto selection = this->heliostats.getSelection();
					for (auto heliostat : selection) {
						heliostat->drawWorld();
					}
				}

				//----------
				std::vector<shared_ptr<Heliostats::Heliostat>> Heliostats::getHeliostats() {
					return this->heliostats.getSelection();
				}

				//----------
				void Heliostats::add(shared_ptr<Heliostat> heliostat) {
					this->heliostats.add(heliostat);
				}

				//----------
				void Heliostats::removeHeliostat(shared_ptr<Heliostat> heliostat) {
					this->heliostats.remove(heliostat);
				}

#pragma mark Heliostat
				//----------
				Heliostats::Heliostat::Heliostat() {
					RULR_SERIALIZE_LISTENERS;
				}

				//----------
				string Heliostats::Heliostat::getDisplayString() const {
					stringstream ss;
					ss << "Position : " << this->position << endl;
					ss << "Rotation : " << this->rotationY;
					return ss.str();
				}

				//----------
				void Heliostats::Heliostat::drawWorld() {
					ofPushMatrix();
					{
						ofTranslate(this->position);
						ofRotateDeg(this->rotationY.get(), 0, 1, 0);
						ofDrawAxis(0.1f);
						ofDrawBitmapString(this->name, glm::vec3());

						ofTranslate(0.0f, -0.095f, 0.0f);
						ofPushStyle();
						{
							ofNoFill();
							ofSetColor(this->color);

							static ofBoxPrimitive * box = 0;
							if(!box) {
								box = new ofBoxPrimitive();
								box->setWidth(0.22f);
								box->setHeight(0.29f);
								box->setDepth(0.13f);
								box->setResolution(1);
								box->setUseVbo(true);
							}
							box->drawWireframe();
						}
						ofPopStyle();
					}
					ofPopMatrix();

					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorld();
					}
				}

				//----------
				void Heliostats::Heliostat::serialize(nlohmann::json & json) {
					Utils::serialize(json, this->name);
					Utils::serialize(json, this->position);
					Utils::serialize(json, this->rotationY);
					Utils::serialize(json, this->axis1Servo);
					Utils::serialize(json, this->axis2Servo);
					this->captures.serialize(json["captures"]);
				}

				//----------
				void Heliostats::Heliostat::deserialize(const nlohmann::json & json) {
					Utils::deserialize(json, this->name);
					Utils::deserialize(json, this->position);
					Utils::deserialize(json, this->rotationY);
					Utils::deserialize(json, this->axis1Servo);
					Utils::deserialize(json, this->axis2Servo);
					this->captures.deserialize(json["captures"]);
				}

				//----------
				void Heliostats::Heliostat::calcPosition(float axis2ToPlaneLength /*= 0.15f*/) {
					auto captures = this->captures.getSelection();
					if (!captures.empty()) {
						glm::vec3 accumulator;
						for (auto capture : captures) {
							auto backPosition = capture->mirrorPlane.meanWorldPoint
								- capture->mirrorPlane.plane.getNormal()
								* axis2ToPlaneLength;

							accumulator += backPosition;
						}
						this->position = accumulator / captures.size();
					}
				}

				//----------
				ofxCvGui::ElementPtr Heliostats::Heliostat::getDataDisplay() {
					auto element = ofxCvGui::makeElement();

					auto widget0 = make_shared<ofxCvGui::Widgets::EditableValue<string>>(this->name);
					{
						element->addChild(widget0);
					}

					auto widget1 = make_shared<ofxCvGui::Widgets::EditableValue<glm::vec3>>(this->position);
					{
						element->addChild(widget1);
					}

					auto widget2 = make_shared<ofxCvGui::Widgets::Slider>(this->rotationY);
					{
						element->addChild(widget2);
					}

					auto widget3 = make_shared<ofxCvGui::Widgets::EditableValue<int>>(this->axis1Servo);
					{
						element->addChild(widget3);
					}

					auto widget4 = make_shared<ofxCvGui::Widgets::EditableValue<int>>(this->axis2Servo);
					{
						element->addChild(widget4);
					}

					auto widget5 = make_shared<ofxCvGui::Widgets::LiveValue<size_t>>("Capture count : ", [this]() {
						return this->captures.size();
					});
					{
						element->addChild(widget5);
					}

					element->onBoundsChange += [this, widget0, widget1, widget2, widget3, widget4, widget5](ofxCvGui::BoundsChangeArguments & args) {
						auto bounds = args.localBounds;
						bounds.height = 40.0f;
						widget0->setBounds(bounds);
						bounds.y += bounds.height;
						widget1->setBounds(bounds);
						bounds.y += bounds.height;
						widget2->setBounds(bounds);
						bounds.y += bounds.height;
						widget3->setBounds(bounds);
						bounds.y += bounds.height;
						widget4->setBounds(bounds);
						bounds.y += bounds.height;
						widget5->setBounds(bounds);
					};
					
					element->setHeight(240.0f);

					return element;
				}
			}
		}
	}
}
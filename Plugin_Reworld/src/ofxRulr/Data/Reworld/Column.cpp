#include "pch_Plugin_Reworld.h"
#include "Column.h"
#include "ofxRulr/Nodes/Reworld/Installation.h"
#include "ofxRulr/Nodes/Reworld/ModuleView.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//----------
			Column::Column()
			{
				// Common init
				{
					this->onSerialize += [this](nlohmann::json& json) {
						this->serialize(json);
						};
					this->onDeserialize += [this](const nlohmann::json& json) {
						this->deserialize(json);
						};
					this->onPopulateInspector += [this](ofxCvGui::InspectArguments& args) {
						args.inspector->addParameterGroup(this->parameters);
						};

					this->rigidBody = make_shared<Nodes::Item::RigidBody>();
					this->rigidBody->init();

					this->rigidBody->onDrawObjectAdvanced += [this](DrawWorldAdvancedArgs& args) {
						this->drawObjectAdvanced(args);
						};
				}
			}

			//----------
			string
				Column::getTypeName() const
			{
				return "Column";
			}

			//----------
			string
				Column::getDisplayString() const
			{
				return ofToString(this->rigidBody->getName());
			}

			//----------
			void
				Column::drawWorldAdvanced(DrawWorldAdvancedArgs& args)
			{
				this->rigidBody->drawWorldAdvanced(args);
			}

			//----------
			void
				Column::drawObjectAdvanced(DrawWorldAdvancedArgs& args)
			{
				auto argsCustom = args;
				auto selectedModules = this->modules.getSelection();
				auto labelDraws = static_cast<Nodes::Reworld::Installation::LabelDraws*>(argsCustom.custom);

				for (auto module : selectedModules) {
					// Enable and disable the caption for the RigidBody
					argsCustom.enableGui = ofxRulr::isActive(Nodes::Reworld::ModuleView::isSelected(module.get()), labelDraws->module.get());
					module->drawWorldAdvanced(argsCustom);
				}
			}

			//----------
			void
				Column::setParent(Nodes::Reworld::Installation* parent)
			{
				this->parent = parent;
			}

			//----------
			void
				Column::init()
			{
				// Init modules
				{
					auto modules = this->modules.getAllCaptures();
					for (auto module : modules) {
						module->parentSelection = &this->ourSelection;
						module->setParent(this);
						module->init();
					}
				}

				// Color
				{
					auto color = Utils::AbstractCaptureSet::BaseCapture::color;
					this->rigidBody->setColor(color);
				}

				// Name
				{
					auto name = "Column " + ofToString(this->parameters.index.get());
					this->rigidBody->setName(name);
				}
			}

			//----------
			void
				Column::update()
			{
				auto modules = this->getAllModules();
				for (auto module : modules) {
					module->update();
				}
			}

			//----------
			void
				Column::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);

				this->modules.serialize(json["modules"]);
				this->rigidBody->serialize(json["rigidBody"]);
			}

			//----------
			void
				Column::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);

				if (json.contains("modules")) {
					this->modules.deserialize(json["modules"]);
				}

				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}

				this->init();
			}

			//----------
			void
				Column::build(int columnIndex, int countY, float pitch)
			{
				this->parameters.index.set(columnIndex);

				this->modules.resize(countY, []() { return make_shared<Module>(); });

				// incremental transform
				glm::mat4 runningTransform = glm::translate(glm::vec3(0, -pitch / 2.0f, 0));

				auto allModules = this->modules.getAllCaptures();
				for (int i = 0; i < countY; i++) {
					auto module = allModules[i];
					module->positionInColumn->setTransform(runningTransform);
					module->parameters.ID.set(i + 1);

					// Increment transform
					runningTransform = glm::translate(glm::vec3(0, -pitch, 0)) * runningTransform;
				}

				this->init();
			}

			//----------
			glm::mat4
				Column::getAbsoluteTransform() const
			{
				return this->parent->getTransform()
					* this->rigidBody->getTransform();
			}

			//----------
			vector<shared_ptr<Module>>
				Column::getAllModules() const
			{
				return this->modules.getAllCaptures();
			}

			//----------
			shared_ptr<Module>
				Column::getModuleByIndex(ModuleIndex moduleIndex, bool selectedOnly) const
			{
				auto modules = this->getAllModules();
				if (moduleIndex >= modules.size() || moduleIndex < 0) {
					throw(ofxRulr::Exception("Index " + ofToString(moduleIndex) + " module out of bounds"));
				}

				auto module = modules[moduleIndex];
				if (selectedOnly && !module->isSelected()) {
					return shared_ptr<Module>();
				}

				return module;
			}

			//---------
			map<Data::Reworld::ModuleIndex, shared_ptr<Data::Reworld::Module>>
				Column::getSelectedModulesByIndex() const
			{
				map<Data::Reworld::ModuleIndex, shared_ptr<Data::Reworld::Module>> modulesByIndex;

				auto modules = this->getAllModules();
				for (Data::Reworld::ModuleIndex moduleIndex = 0; moduleIndex < modules.size(); moduleIndex++) {
					auto module = modules[moduleIndex];
					if (module->isSelected()) {
						modulesByIndex.emplace(moduleIndex, module);
					}
				}

				return modulesByIndex;
			}

			//----------
			ofxCvGui::ElementPtr
				Column::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				stack->add(make_shared<ofxCvGui::Widgets::LiveValue<int>>("Column", [this]() {
					return this->parameters.index.get();
					}));

				// Select column
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Select column"
						, [this]() {
							if (this->parentSelection) {
								return this->parentSelection->isSelected(this);
							}
							else {
								return false;
							}
						}
						, [this](bool value) {
							if (this->parentSelection) {
								this->parentSelection->select(this);
							}
							else {
								ofLogError() << "EditSelection not initialised";
							}
							});
					button->setDrawGlyph(u8"\uf03a");
					stack->add(button);
				}

				// Select the rigidBody
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Column transform"
						, [this]() {
							return this->isBeingInspected();
						}
						, [this](bool value) {
							if (value) {
								ofxCvGui::inspect(this->rigidBody);
							}
							});
					button->setDrawable([this](ofxCvGui::DrawArguments& args) {
						ofRectangle bounds;
						bounds.setFromCenter(args.localBounds.getCenter(), 32, 32);
						this->rigidBody->getIcon()->draw(bounds);
						});
					stack->add(button);
				}

				return stack;
			}
		}
	}
}
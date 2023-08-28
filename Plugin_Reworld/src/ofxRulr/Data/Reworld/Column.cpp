#include "pch_Plugin_Reworld.h"
#include "Column.h"

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
				auto panels = this->panels.getSelection();
				for (auto panel : panels) {
					panel->drawWorldAdvanced(args);
				}
			}

			//----------
			vector<glm::vec3>
				Column::getVertices() const
			{
				vector<glm::vec3> vertices;
				auto transform = this->rigidBody->getTransform();

				auto panels = this->panels.getSelection();
				for (auto panel : panels) {
					auto panelVertices = panel->getVertices();
					for (const auto & panelVertex : panelVertices) {
						vertices.push_back(ofxCeres::VectorMath::applyTransform(transform, panelVertex));
					}
				}

				return vertices;
			}

			//----------
			void
				Column::init()
			{
				// Sub selections
				{
					auto panels = this->panels.getAllCaptures();
					for (auto panel : panels) {
						panel->parentSelection = &this->ourSelection;
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
				Column::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);

				this->panels.serialize(json["panels"]);
				this->rigidBody->serialize(json["rigidBody"]);
			}

			//----------
			void
				Column::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);

				if (json.contains("panels")) {
					this->panels.deserialize(json["panels"]);
				}

				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}

				this->init();
			}

			//----------
			void
				Column::build(const BuildParameters& buildParameters, const Panel::BuildParameters& panelBuildParameters, int columnIndex)
			{
				this->panels.clear();

				// pull the parameters
				const auto countY = buildParameters.countY.get();
				const auto yStart = buildParameters.yStart.get();
				const auto verticalStride = buildParameters.verticalStride.get();

				this->parameters.index.set(columnIndex);

				// incremental transform
				glm::mat4 currentTransform = glm::translate(glm::vec3(0, yStart, 0));
				
				int targetIndexOffset = 1;

				for (int i = 0; i < countY; i++) {
					auto panel = make_shared<Panel>();
					panel->parentSelection = &this->ourSelection;
					panel->build(panelBuildParameters, targetIndexOffset);
					panel->rigidBody->setTransform(currentTransform);
					this->panels.add(panel);

					// Increment transform
					currentTransform *= glm::translate(glm::vec3(0, verticalStride, 0));

					// Increment targetIndex
					targetIndexOffset += panel->portals.size();
				}

				this->init();
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
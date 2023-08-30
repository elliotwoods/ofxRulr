#include "pch_Plugin_Reworld.h"
#include "Panel.h"
#include "ofxRulr/Nodes/Reworld/Installation.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//----------
			Panel::Panel()
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
				Panel::getTypeName() const
			{
				return "Panel";
			}

			//----------
			string
				Panel::getDisplayString() const
			{
				return ofToString(this->rigidBody->getName());
			}

			//---------
			void
				Panel::drawWorldAdvanced(DrawWorldAdvancedArgs& args)
			{
				this->rigidBody->drawWorldAdvanced(args);
			}

			//----------
			void
				Panel::drawObjectAdvanced(DrawWorldAdvancedArgs& args)
			{
				// Draw outline
				{
					ofPushStyle();
					{
						ofNoFill();

						if (this->isBeingInspected()) {
							ofSetColor(ofxCvGui::Utils::getBeatingSelectionColor());
						}
						else {
							ofSetColor(Utils::AbstractCaptureSet::BaseCapture::color.get());
						}

						{
							const auto width = this->parameters.width.get();
							const auto height = this->parameters.height.get();
							ofDrawRectangle(-width / 2.0f, -height / 2.0f, width, height);
						}
					}
					ofPopStyle();
				}

				// Draw portals
				{
					auto argsCustom = args;

					auto portals = this->portals.getSelection();
					for (auto portal : portals) {
						argsCustom.enableGui = isActive(this->ourSelection.selection == portal.get()
							, ((Nodes::Reworld::Installation::LabelDraws*)argsCustom.custom)->portal);
						portal->drawWorldAdvanced(argsCustom);
					}
				}
			}

			//----------
			vector<glm::vec3>
				Panel::getVertices() const
			{
				vector<glm::vec3> vertices;
				auto transform = this->rigidBody->getTransform();

				auto portals = this->portals.getSelection();
				for (auto portal : portals) {
					auto panelVertices = portal->getVertices();
					for (const auto& portalVertex : panelVertices) {
						vertices.push_back(ofxCeres::VectorMath::applyTransform(transform, portalVertex));
					}
				}

				return vertices;
			}

			//----------
			void
				Panel::init()
			{
				// Sub selections
				{
					auto portals = this->portals.getAllCaptures();
					for (auto portal : portals) {
						portal->parentSelection = &this->ourSelection;
					}
				}

				// Color
				{
					auto color = Utils::AbstractCaptureSet::BaseCapture::color;
					this->rigidBody->setColor(color);
				}

				// Name
				{
					auto portals = this->portals.getSelection();
					if (!portals.empty()) {
						auto firstID = portals.front()->parameters.target.get();
						auto lastID = portals.back()->parameters.target.get();
						auto name = ofToString(firstID) + "..." + ofToString(lastID);

						rigidBody->setName(name);
					}
				}
			}

			//----------
			void
				Panel::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);

				this->portals.serialize(json["portals"]);
				this->rigidBody->serialize(json["rigidBody"]);
			}

			//----------
			void
				Panel::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);

				if (json.contains("portals")) {
					this->portals.deserialize(json["portals"]);
				}

				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}

				this->init();
			}

			//----------
			void
				Panel::build(const BuildParameters& buildParameters, int targetIndexOffset)
			{
				this->portals.clear();

				const auto pitch = buildParameters.pitch.get();
				const auto countX = buildParameters.countX.get();
				const auto countY = buildParameters.countY.get();

				glm::vec3 offset(-pitch * (countX - 1) / 2.0f, pitch * (countX - 1) / 2.0f, 0.0f);

				int targetIndex = targetIndexOffset;
				for (int j = 0; j < countY; j++) {
					for (int i = 0; i < countX; i++) {
						auto portalPosition = glm::vec3(i * pitch, -j * pitch, 0.0f) + offset;
						auto portal = make_shared<Portal>();
						portal->build(targetIndex);
						portal->rigidBody->setTransform(glm::translate(portalPosition));
						this->portals.add(portal);
						
						targetIndex++;
					}
				}

				this->init();
			}

			//----------
			ofxCvGui::ElementPtr
				Panel::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				stack->add(make_shared<ofxCvGui::Widgets::LiveValue<string>>("Panel", [this]() {
					return this->rigidBody->getName();
					}));

				// Select panel
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Select panel"
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
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Panel transform"
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
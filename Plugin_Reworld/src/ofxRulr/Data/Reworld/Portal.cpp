#include "pch_Plugin_Reworld.h"
#include "Panel.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//---------
			shared_ptr<ofTexture> Portal::panelPreview;

			//---------
			Portal::Portal()
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

				// Setup the mipped panel preview
				{
					if (!panelPreview) {
						auto wasArbTex = ofGetUsingArbTex();
						ofDisableArbTex();
						{
							panelPreview = make_shared<ofTexture>();
							panelPreview->enableMipmap();
							panelPreview->loadData(ofxAssets::image("ofxRulr::Reworld::Shroud").getPixels());
							panelPreview->setTextureWrap(GL_REPEAT, GL_REPEAT);
							panelPreview->setTextureMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST);
						}
						if (wasArbTex) ofEnableArbTex();
					}
				}
			}

			//---------
			string
				Portal::getTypeName() const
			{
				return "Portal";
			}

			//---------
			string
				Portal::getDisplayString() const
			{
				return ofToString(this->rigidBody->getName());
			}

			//---------
			void
				Portal::build(int targetIndex)
			{
				this->parameters.target.set(targetIndex);
				this->init();
			}


			//---------
			void
				Portal::drawWorldAdvanced(DrawWorldAdvancedArgs& args)
			{
				this->rigidBody->drawWorldAdvanced(args);
			}

			//---------
			void
				Portal::drawObjectAdvanced(DrawWorldAdvancedArgs& args)
			{
				Portal::panelPreview->draw(-REWORLD_PORTAL_SHROUD_SIZE / 2.0f
					, -REWORLD_PORTAL_SHROUD_SIZE / 2.0f
					, REWORLD_PORTAL_SHROUD_SIZE
					, REWORLD_PORTAL_SHROUD_SIZE);
			}

			//---------
			vector<glm::vec3>
				Portal::getVertices() const
			{
				const auto a = 0.032993;
				const auto b = 0.058109;
				return {
					{ a, b, 0.0f }
					, {b, a, 0.0f}
					, { -a, b, 0.0f }
					, {b, -a, 0.0f}
					, { a, -b, 0.0f }
					, {-b, a, 0.0f}
					, { -a, -b, 0.0f }
					, {-b, -a, 0.0f}
				};
			}

			//----------
			void
				Portal::init()
			{
				// Color
				{
					auto color = Utils::AbstractCaptureSet::BaseCapture::color;
					this->rigidBody->setColor(color);
				}

				// Name
				{
					auto name = "Portal " + ofToString(this->parameters.target.get());
					rigidBody->setName(name);
				}
			}

			//----------
			void
				Portal::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);

				this->rigidBody->serialize(json["rigidBody"]);
			}

			//----------
			void
				Portal::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);

				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}

				this->init();
			}

			//----------
			ofxCvGui::ElementPtr
				Portal::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				stack->add(make_shared<ofxCvGui::Widgets::LiveValue<int>>("Target", [this]() {
					return this->parameters.target.get();
					}));

				// Select portal
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Select portal"
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
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Portal transform"
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
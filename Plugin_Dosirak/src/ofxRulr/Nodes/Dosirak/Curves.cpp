#include "pch_Plugin_Dosirak.h"
#include "Curves.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			//----------
			Curves::Curves()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Curves::getTypeName() const
			{
				return "Dosirak::Curves";
			}

			//----------
			void
				Curves::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->rigidBody = make_shared<Item::RigidBody>();
				this->rigidBody->init();

				this->manageParameters(this->parameters);

				// Setup the panel
				{
					auto panel = ofxCvGui::Panels::makeWidgets();

					panel->addLiveValue<size_t>("Curve count", [this]() {
						return (bool)this->curves.size();
						});

					// Curve list
					{
						auto widget = ofxCvGui::makeElement();
						widget->setHeight(300);
						widget->onDraw += [this](ofxCvGui::DrawArguments& args) {
							ofPushMatrix();
							{
								for (const auto& curve : this->curves) {
									ofTranslate(0, 25, 0);
									ofPushStyle();
									{
										ofSetColor(curve.second.color);
										ofDrawRectangle(3, 3, 17, 17);
									}
									ofPopStyle();

									string previewString = curve.first + " : " + ofToString(curve.second.points.size());
									ofxCvGui::Utils::drawText(previewString, 20, 20, false);
								}
							}
							ofPopMatrix();
						};
						panel->add(widget);
					}

					this->panel = panel;
				}

				// Draw object function
				{
					this->rigidBody->onDrawObject += [this]() {
						if (this->previewStale) {
							this->updatePreview();
						}

						ofPushMatrix();
						{
							auto scale = this->parameters.scale.get();
							ofScale(scale, scale, scale);
							for (const auto& curve : this->curves) {
								curve.second.drawPreview();
							}
						}
						ofPopMatrix();
					};
				}

				this->rigidBody->onTransformChange.addListener([this]() {
					this->newCurves.incoming = true;
					}, this);
			}

			//----------
			void
				Curves::update()
			{
				// Update onNewFrame pattern
				{
					this->newCurves.thisFrame = false;
					if (this->newCurves.incoming) {
						this->newCurves.thisFrame = true;
						this->newCurves.incoming = false;
					}
				}

				// What to do isFrameNew
				if (this->newCurves.thisFrame) {
					this->previewStale = true;
					this->onNewCurves.notifyListeners();
				}

				// Update preview if stale
				if (this->previewStale) {
					this->updatePreview();
				}
			}

			//----------
			void
				Curves::drawWorldStage()
			{
				this->rigidBody->drawWorldStage();
			}

			//----------
			ofxCvGui::PanelPtr
				Curves::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Curves::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;
				{
					auto widget = inspector->addSubMenu("Rigid Body", this->rigidBody);
					widget->setHeight(100.0f);
				}
			}

			//----------
			void
				Curves::serialize(nlohmann::json& json) const
			{
				this->curves.serialize(json["curves"]);
				this->rigidBody->serialize(json["rigidBody"]);
			}

			//----------
			void
				Curves::deserialize(const nlohmann::json& json)
			{
				if (json.contains("curves")) {
					this->curves.deserialize(json["curves"]);
					this->previewStale = true;
				}

				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}
			}

			//----------
			void
				Curves::setCurves(const Data::Dosirak::Curves& curves)
			{
				this->curves = curves;
				this->newCurves.incoming = true;
				this->previewStale = true; // we might not get to update before next draw to screen
			}

			//----------
			const Data::Dosirak::Curves&
				Curves::getCurvesRaw() const
			{
				return this->curves;
			}

			//----------
			Data::Dosirak::Curves
				Curves::getCurvesTransformed() const
			{
				const auto scale = this->parameters.scale.get();
				auto transform = this->rigidBody->getTransform() * glm::scale(glm::vec3(scale, scale, scale));
				return this->curves.getTransformed(transform);
			}
			
			//----------
			void
				Curves::updatePreview()
			{
				for (auto& it : this->curves) {
					it.second.updatePreview();
				}
				this->previewStale = false;
			}

		}
	}
}
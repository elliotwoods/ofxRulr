#include "pch_Plugin_Reworld.h"
#include "Module.h"
#include "ofxRulr/Nodes/Reworld/Installation.h"
#include "ofxRulr/Nodes/Reworld/ModuleView.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//---------
			Module::Module()
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

					this->positionInColumn = make_shared<Nodes::Item::RigidBody>();
					this->positionInColumn->init();

					this->positionInColumn->onDrawObjectAdvanced += [this](DrawWorldAdvancedArgs& args) {
						this->drawObjectAdvanced(args);
						};
				}
			}

			//---------
			string
				Module::getTypeName() const
			{
				return "Module";
			}

			//---------
			string
				Module::getDisplayString() const
			{
				return ofToString(this->positionInColumn->getName());
			}

			//---------
			void
				Module::drawWorldAdvanced(DrawWorldAdvancedArgs& args)
			{
				this->positionInColumn->drawWorldAdvanced(args);
			}

			//---------
			void
				Module::drawObjectAdvanced(DrawWorldAdvancedArgs& args)
			{
				auto labelDraws = static_cast<Nodes::Reworld::Installation::LabelDraws*>(args.custom);

				ofPushStyle();
				{
					ofNoFill();
					ofDrawCircle({ 0, 0 }, 0.05f);
				}
				ofPopStyle();
			}

			//----------
			void
				Module::setParent(Column* parent)
			{
				this->parent = parent;
			}

			//----------
			Nodes::Reworld::Installation *
				Module::getInstallation() const
			{
				return this->parent->parent;
			}

			//----------
			void
				Module::init()
			{
				// Color
				{
					auto color = Utils::AbstractCaptureSet::BaseCapture::color;
					this->positionInColumn->setColor(color);
				}

				// Name
				{
					auto name = "Module " + ofToString(this->parameters.ID.get());
					positionInColumn->setName(name);
				}
			}

			//----------
			void
				Module::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);

				this->positionInColumn->serialize(json["positionInColumn"]);
			}

			//----------
			void
				Module::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);

				if (json.contains("positionInColumn")) {
					this->positionInColumn->deserialize(json["positionInColumn"]);
				}

				this->init();
			}


			//----------
			glm::mat4
				Module::getBulkTransform() const
			{
				return this->parent->getAbsoluteTransform()
					* this->positionInColumn->getTransform();
			}

			//----------
			glm::mat4
				Module::getTransformOffset() const
			{
				glm::vec3 translation{
					this->parameters.calibrationParameters.transformOffset.translation.x.get()
					, this->parameters.calibrationParameters.transformOffset.translation.y.get()
					, this->parameters.calibrationParameters.transformOffset.translation.z.get()
				};

				glm::vec3 rotationVector{
					this->parameters.calibrationParameters.transformOffset.rotationVector.x.get()
					, this->parameters.calibrationParameters.transformOffset.rotationVector.y.get()
					, this->parameters.calibrationParameters.transformOffset.rotationVector.z.get()
				};

				return ofxCeres::VectorMath::createTransform(translation, rotationVector);
			}

			//----------
			glm::mat4
				Module::getAbsoluteTransform() const
			{
				return this->getBulkTransform()
					* this->getTransformOffset();
			}

			//----------
			Models::Reworld::Module
				Module::getModel() const
			{
				Models::Reworld::Module model;

				// Calibration
				{
					model.bulkTransform = this->getBulkTransform();

					// Transform offset
					{
						glm::vec3 translation{
						this->parameters.calibrationParameters.transformOffset.translation.x.get()
						, this->parameters.calibrationParameters.transformOffset.translation.y.get()
						, this->parameters.calibrationParameters.transformOffset.translation.z.get()
						};

						glm::vec3 rotationVector{
							this->parameters.calibrationParameters.transformOffset.rotationVector.x.get()
							, this->parameters.calibrationParameters.transformOffset.rotationVector.y.get()
							, this->parameters.calibrationParameters.transformOffset.rotationVector.z.get()
						};
						model.transformOffset.translation = translation;
						model.transformOffset.rotation = rotationVector;
					}

					// Axis angle offets
					{
						model.axisAngleOffsets.A = this->parameters.calibrationParameters.axisAngleOffsets.A.get();
						model.axisAngleOffsets.B = this->parameters.calibrationParameters.axisAngleOffsets.B.get();
					}

					// Installation-wide parameters
					{
						const auto& physicalParameters = this->getInstallation()->getPhysicalParameters();
						model.installationParameters.interPrismDistance = physicalParameters.interPrismDistanceMM.get() / 1000.0f;
						model.installationParameters.prismAngleRadians = physicalParameters.prismAngle.get() * DEG_TO_RAD;
						model.installationParameters.ior = physicalParameters.ior.get();
					}
				}

				return model;
			}

			//----------
			ofxCvGui::ElementPtr
				Module::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				stack->add(make_shared<ofxCvGui::Widgets::LiveValue<int>>(this->parameters.ID));

				// Select module
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Select module"
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
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Position in Column"
						, [this]() {
							return this->isBeingInspected();
						}
						, [this](bool value) {
							if (value) {
								ofxCvGui::inspect(this->positionInColumn);
							}
							});
					button->setDrawable([this](ofxCvGui::DrawArguments& args) {
						ofRectangle bounds;
						bounds.setFromCenter(args.localBounds.getCenter(), 32, 32);
						this->positionInColumn->getIcon()->draw(bounds);
						});
					stack->add(button);
				}

				return stack;
			}
		}
	}
}
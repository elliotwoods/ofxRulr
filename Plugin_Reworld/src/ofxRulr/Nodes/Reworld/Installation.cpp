#include "pch_Plugin_Reworld.h"
#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//---------
			Installation::Installation()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string
				Installation::getTypeName() const
			{
				return "Reworld::Installation";
			}

			//---------
			void
				Installation::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				

				this->rigidBody = make_shared<Nodes::Item::RigidBody>();
				this->rigidBody->onDrawObjectAdvanced += [this](DrawWorldAdvancedArgs& args) {
					this->drawObjectAdvanced(args);
				};
				this->onDrawWorldAdvanced += [this](DrawWorldAdvancedArgs& args) {
					this->rigidBody->drawWorldAdvanced(args);
				};

				this->manageParameters(this->parameters);

				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					panel->addTitle("Camera positions:", ofxCvGui::Widgets::Title::Level::H3);
					this->columns.populateWidgets(panel);
					this->panel = panel;
				}
			}

			//---------
			void
				Installation::update()
			{

			}

			//---------
			void
				Installation::drawObjectAdvanced(DrawWorldAdvancedArgs& args)
			{
				auto columns = this->columns.getSelection();
				for (auto column : columns) {
					column->drawWorldAdvanced(args);
				}
			}

			//---------
			void
				Installation::serialize(nlohmann::json& json)
			{
				this->columns.serialize(json["columns"]);
				this->rigidBody->serialize(json["rigidBody"]);
			}

			//---------
			void
				Installation::deserialize(const nlohmann::json& json)
			{
				if (json.contains("columns")) {
					this->columns.deserialize(json["columns"]);
				}

				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}

				this->initColumns();
			}

			//---------
			void
				Installation::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Build", [this]() {
					this->build();
					}, OF_KEY_RETURN);
			}

			//---------
			ofxCvGui::PanelPtr
				Installation::getPanel()
			{
				return this->panel;
			}

			//---------
			void
				Installation::build()
			{
				this->columns.clear();

				// pull the parameters
				const auto horizontalStride = this->parameters.builder.installation.horizontalStride.get();
				const auto angleBetweenPanels = this->parameters.builder.installation.angleBetweenColumns.get() * DEG_TO_RAD;
				const auto countX = this->parameters.builder.installation.countX.get();

				// Create the relative transform between columns
				auto transformBetweenColumns = glm::translate(glm::vec3(horizontalStride / 2.0f, 0.0f, 0.0f))
					* glm::rotate((float)angleBetweenPanels, glm::vec3(0, 1, 0))
					* glm::translate(glm::vec3(horizontalStride / 2.0f, 0.0f, 0.0f));
				
				// Build the columns sequentially
				glm::mat4 currentTransform;
				for (int i = 0; i < countX; i++) {
					auto column = make_shared<Data::Reworld::Column>();
					column->build(this->parameters.builder.column, this->parameters.builder.panel, i);
					column->rigidBody->setTransform(currentTransform);
					column->parentSelection = &this->ourSelection;
					this->columns.add(column);

					// add to the transform each step
					currentTransform = currentTransform * transformBetweenColumns;
				}

				this->initColumns();
			}

			//---------
			void
				Installation::initColumns()
			{
				// Sub selections
				{
					auto columns = this->columns.getAllCaptures();
					for (auto column : columns) {
						column->parentSelection = &this->ourSelection;
					}
				}
			}

			//---------
			vector<glm::vec3>
				Installation::getVertices() const
			{
				vector<glm::vec3> allVertices;
				auto transform = this->rigidBody->getTransform();

				// Gather from columns
				auto columns = this->columns.getSelection();
				for(auto column : columns) {
					auto columnVertices = column->getVertices();
					
					// transform vertices into world space as we add them
					for (auto vertex : columnVertices) {
						allVertices.push_back(ofxCeres::VectorMath::applyTransform(transform, vertex));
					}
				}

				return allVertices;
			}
		}
	}
}
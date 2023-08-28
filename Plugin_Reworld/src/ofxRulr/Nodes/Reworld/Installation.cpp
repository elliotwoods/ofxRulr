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
				return "Installation";
			}

			//---------
			void
				Installation::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_ADVANCED_LISTENER;

				this->manageParameters(this->parameters);
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

			}

			//---------
			void
				Installation::deserialize(const nlohmann::json& json)
			{
				if (json.contains("columns")) {
					this->columns.deserialize(json["columns"]);
				}
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
					this->columns.add(column);
					column->build(this->parameters.builder.column, this->parameters.builder.panel, i);
					column->setTransform(currentTransform);

					// add to the transform each step
					currentTransform = currentTransform * transformBetweenColumns;
				}
			}

			//---------
			vector<glm::vec3>
				Installation::getVertices() const
			{
				vector<glm::vec3> allVertices;
				auto transform = this->getTransform();

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
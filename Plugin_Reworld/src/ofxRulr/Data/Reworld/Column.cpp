#include "pch_Plugin_Reworld.h"
#include "Column.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//----------
			Column::Column()
			{
				RULR_RIGIDBODY_DRAW_OBJECT_ADVANCED_LISTENER;
				this->manageParameters(this->parameters);
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
				return ofToString(Nodes::Base::getName());
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
				auto transform = this->getTransform();

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
					this->panels.add(panel);
					panel->build(panelBuildParameters, targetIndexOffset);
					panel->setTransform(currentTransform);

					// Increment transform
					currentTransform *= glm::translate(glm::vec3(0, verticalStride, 0));

					// Increment targetIndex
					targetIndexOffset += panel->portals.size();
				}

				Nodes::Base::setColor(Utils::AbstractCaptureSet::BaseCapture::color);
				this->setName("Column " + ofToString(this->parameters.index.get()));
			}
		}
	}
}
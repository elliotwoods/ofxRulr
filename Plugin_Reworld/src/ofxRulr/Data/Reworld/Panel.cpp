#include "pch_Plugin_Reworld.h"
#include "Panel.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//----------
			Panel::Panel()
			{
				RULR_RIGIDBODY_DRAW_OBJECT_ADVANCED_LISTENER;
				this->manageParameters(this->parameters);
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
				return ofToString(Nodes::Base::getName());
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
					auto portalDrawArgs = args;
					portalDrawArgs.enableGui = false;

					auto portals = this->portals.getSelection();
					for (auto portal : portals) {
						portal->drawWorldAdvanced(portalDrawArgs);
					}
				}
			}

			//----------
			vector<glm::vec3>
				Panel::getVertices() const
			{
				vector<glm::vec3> vertices;
				auto transform = this->getTransform();

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

						portal->setPosition(portalPosition);
						this->portals.add(portal);
						
						targetIndex++;
					}
				}

				Nodes::Base::setColor(Utils::AbstractCaptureSet::BaseCapture::color);

				// Set name from indices
				{
					auto portals = this->portals.getSelection();
					if (!portals.empty()) {
						auto firstID = portals.front()->parameters.target.get();
						auto lastID= portals.back()->parameters.target.get();
						this->setName(ofToString(firstID) + "..." + ofToString(lastID));
					}
				}
			}
		}
	}
}
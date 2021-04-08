#include "pch_RulrNodes.h"
#include "Lighting.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			//----------
			Lighting::Lighting() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Lighting::getTypeName() const {
				return "Render::Lighting";
			}

			//----------
			void Lighting::init() {
				this->manageParameters(this->parameters);
			}

			//----------
			void Lighting::customBegin() {
				switch (this->parameters.lightMode.get()) {
				case LightMode::Point:
				{
					this->light.setPointLight();
				}
					break;
				case LightMode::Spot:
				{
					this->light.setSpotlight(this->parameters.spotLight.cutoffAngle);
				}
					break;
				case LightMode::Directional:
				{
					this->light.setDirectional();
				}
					break;
				default:
					return;
				}

				this->light.setAmbientColor(this->parameters.color.ambient);
				this->light.setDiffuseColor(this->parameters.color.diffuse);
				this->light.setSpecularColor(this->parameters.color.specular);

				this->applyTransformToNode(this->light);

				this->light.enable();
			}

			//----------
			void Lighting::customEnd() {
				this->light.disable();
			}
		}
	}
}
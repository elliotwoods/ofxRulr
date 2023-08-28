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
				RULR_RIGIDBODY_DRAW_OBJECT_ADVANCED_LISTENER;
				this->manageParameters(this->parameters);

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
				return ofToString("y = " + ofToString(this->getPosition().y));
			}

			//---------
			void
				Portal::build(int targetIndex)
			{
				this->parameters.target.set(targetIndex);
				Nodes::Base::setColor(Utils::AbstractCaptureSet::BaseCapture::color);
				this->setName("Panel " + ofToString(this->parameters.target.get()));
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
		}
	}
}
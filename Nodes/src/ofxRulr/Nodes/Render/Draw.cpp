#include "pch_RulrNodes.h"
#include "Draw.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			//----------
			Draw::Draw() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Draw::getTypeName() const {
				return "Render::Draw";
			}

			//----------
			void Draw::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				this->manageParameters(this->parameters);
			}

			//----------
			void Draw::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {

			}

			//----------
			void Draw::customBegin() {
				glPushAttrib(GL_POLYGON_BIT);
				glEnable(GL_CULL_FACE);

				switch (this->parameters.culling.get()) {
				case Culling::Back:
					glCullFace(GL_BACK);
					break;
				case Culling::Front:
					glCullFace(GL_FRONT);
					break;
				case Culling::FrontAndBack:
					glCullFace(GL_FRONT_AND_BACK);
					break;
				default:
					break;
				}

				switch (this->parameters.frontFace.get()) {
				case FrontFace::CCW:
					glFrontFace(GL_CCW);
					break;
				case FrontFace::CW:
					glFrontFace(GL_CW);
					break;
				default:
					break;
				}
			}

			//----------
			void Draw::customEnd() {
				glDisable(GL_CULL_FACE);
				glPopAttrib();
			}
		}
	}
}
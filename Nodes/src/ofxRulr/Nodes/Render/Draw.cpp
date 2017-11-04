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
				this->manageParameters(this->parameters, false);
			}

			//----------
			void Draw::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				{
					auto multipleChoice = inspector->addMultipleChoice("Culling", { "Back", "Front", "Front+Back" });
					multipleChoice->entangle(this->parameters.culling);
				}

				{
					auto multipleChoice = inspector->addMultipleChoice("Front face", { "CCW", "CW" });
					multipleChoice->entangle(this->parameters.frontFace);
				}
			}

			//----------
			void Draw::customBegin() {
				glPushAttrib(GL_POLYGON_BIT);
				glEnable(GL_CULL_FACE);

				switch (this->parameters.culling.get()) {
				case 0:
					glCullFace(GL_BACK);
					break;
				case 1:
					glCullFace(GL_FRONT);
					break;
				case 2:
					glCullFace(GL_FRONT_AND_BACK);
					break;
				default:
					break;
				}

				switch (this->parameters.frontFace.get()) {
				case 0:
					glFrontFace(GL_CCW);
					break;
				case 1:
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
#include "pch_RulrNodes.h"
#include "Style.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Render {
			//----------
			Style::Style() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string Style::getTypeName() const {
				return "Render::Style";
			}

			//----------
			void Style::init() {
				this->addInput<Style>()->setLoopbackEnabled(false);
			}

			//----------
			void Style::begin() {
				auto inputStyle = this->getInput<Style>();
				if (inputStyle) {
					inputStyle->begin();
				}
				this->customBegin();
			}

			//----------
			void Style::end() {
				this->customEnd();
				auto inputStyle = this->getInput<Style>();
				if (inputStyle) {
					inputStyle->end();
				}
			}
		}
	}
}
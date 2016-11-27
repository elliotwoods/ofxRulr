#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					class Node : public Nodes::Base, public enable_shared_from_this<Node> {
					public:
						Node();
						virtual string getTypeName() const override;
						void init();
						ofxCvGui::ElementPtr getWidget();
					protected:
						virtual shared_ptr<Nodes::Base> getNode() {
							return shared_ptr<Nodes::Base>();
						}

						ofxCvGui::ElementPtr widget;
					};
				}
			}
		}
	}
}
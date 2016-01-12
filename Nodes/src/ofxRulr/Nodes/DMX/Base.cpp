#include "pch_RulrNodes.h"
#include "Base.h"

#include "ofxAssets.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			Base::Base() {
				this->setIcon(ofxAssets::Register::X().getImagePointer("ofxRulr::Nodes::DMX::Base"));
			}
		}
	}
}
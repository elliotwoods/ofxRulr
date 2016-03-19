#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class Receiver : public Nodes::Base {
				Receiver();
				string getTypeName() const override;

			};
		}
	}
}
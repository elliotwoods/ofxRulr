#pragma once

#include "../Utils/Set.h"
#include "../Graph/Node.h"

#include "ofxCvGui/Controller.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class World : public Utils::Set<Graph::Node> {
		public:
			void setupGui(ofxCvGui::Controller &);
		};
	}
}
#pragma once

#include "ofxDigitalEmulsion/Graph/Node.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		class GraphEditor : public Graph::Node {
		public:
			void init() override;
			PinSet getInputPins() const override;
			ofxCvGui::PanelPtr getView() override;
			void update() override;
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;
		};
	}
}
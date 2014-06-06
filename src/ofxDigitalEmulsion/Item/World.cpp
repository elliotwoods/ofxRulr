#include "World.h"

#include "ofxCvGui.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		//-----------
		void World::setupGui(ofxCvGui::Controller & controller) {
			for(auto node : *this) {
				auto nodeView = node->getView();
				controller.add(nodeView);

				nodeView->onMouse += [node] (ofxCvGui::MouseArguments & mouse) {
					if (mouse.isLocalPressed()) {
						ofxCvGui::inspect(* node);
					}
				};

				nodeView->onDraw += [node] (ofxCvGui::DrawArguments & drawArgs) {
					if (ofxCvGui::isBeingInspected(* node)) {
						ofPushStyle();
						ofSetColor(255);
						ofSetLineWidth(3.0f);
						ofNoFill();
						ofRect(drawArgs.localBounds);
						ofPopStyle();
					}
				};
			}
			auto inspector = ofxCvGui::Builder::makeInspector();
			controller.add(inspector);
		}
	}
}
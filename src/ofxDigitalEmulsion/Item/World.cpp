#include "World.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//-----------
		ofxCvGui::Controller * World::gui = 0;

		//-----------
		void World::setupGui(Controller & controller) {
			for(auto node : *this) {
				auto nodeView = node->getView();
				controller.add(nodeView);

				nodeView->onMouse += [node] (MouseArguments & mouse) {
					if (mouse.isLocalPressed() && mouse.button == 0) {
						if (!ofxCvGui::isBeingInspected(* node)) {
							ofxCvGui::inspect(* node);
						}
					}
				};

				nodeView->onDraw += [node] (DrawArguments & drawArgs) {
					if (isBeingInspected(* node)) {
						ofPushStyle();
						ofSetColor(255);
						ofSetLineWidth(3.0f);
						ofNoFill();
						ofRect(drawArgs.localBounds);
						ofPopStyle();
					}
				};

				nodeView->setCaption(node->getTypeName());
			}
			auto inspector = ofxCvGui::Builder::makeInspector();
			controller.add(inspector);
			
			Panels::Inspector::onClear += [] (ElementGroupPtr inspector) {
				inspector->add(Widgets::LiveValueHistory::make("Application fps [Hz]", [] () {
					return ofGetFrameRate();
				}, true));
			};

			World::gui = & controller;
		}

		//-----------
		void World::saveAll() const {
			for(auto node : * this) {
				node->save(node->getDefaultFilename());
			}
		}

		//-----------
		void World::loadAll() {
			for(auto node : * this) {
				node->load(node->getDefaultFilename());
			}
		}

		//-----------
		ofxCvGui::Controller & World::getGuiController() {
			if (World::gui) {
				return * World::gui;
			} else {
				OFXDIGITALEMULSION_FATAL << "No gui attached yet";
			}
		}
	}
}
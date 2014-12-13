#include "World.h"

#include "Summary.h"

#include "../Utils/Exception.h"
#include "../Utils/Initialiser.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Graph {
		//-----------
		ofxCvGui::Controller * World::gui = 0;

		//-----------
		void World::init(Controller & controller) {
			Utils::initialiser.checkInitialised();

			//--
			// INIITALISE NODES
			//--
			//
			set<shared_ptr<Node>> failedNodes;
			for (auto node : *this) {
				bool initSuccess = false;
				try
				{
					node->init();
					initSuccess = true;
				}
				OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT

				if (!initSuccess) {
					failedNodes.insert(node);
				}
			}
			for (auto failedNode : failedNodes) {
				this->remove(failedNode);
			}
			//
			//--




			//--
			// SETUP GUI GRID
			//--
			//
			auto rootGroup = dynamic_pointer_cast<ofxCvGui::Panels::Groups::Grid>(controller.getRootGroup());
			if (rootGroup) {
				//set widths before the rearrangement happens
				rootGroup->onBoundsChange.addListener([rootGroup] (ofxCvGui::BoundsChangeArguments & args) {
					const float inspectorWidth = 300.0f;
					vector<float> widths;
					widths.push_back(args.bounds.getWidth() - inspectorWidth);
					widths.push_back(inspectorWidth);
					rootGroup->setWidths(widths);
				}, -1, this);
			}
			auto gridGroup = MAKE(ofxCvGui::Panels::Groups::Grid);
			gridGroup->setColsCount(1);
			rootGroup->add(gridGroup);
			this->guiGrid = gridGroup;
			//
			//--





			//--
			// NODE GRID
			//--
			//
			auto nodeGrid = MAKE(ofxCvGui::Panels::Groups::Grid);
			guiGrid->add(nodeGrid);
			for(auto node : *this) {
				auto nodeView = node->getView();
				if (nodeView) {
					nodeGrid->add(nodeView);
				}
			}
			//
			//--





			//--
			// SUMMARY VIEW
			//
			auto summary = MAKE(Summary, *this);
			summary->init();
			summary->setName("World");
			this->add(summary); // we intentionally do this after building the Node grid
			gridGroup->add(summary->getView());
			//
			//--
			



			//--
			// INSPECTOR
			//--
			//
			auto inspector = ofxCvGui::Builder::makeInspector();
			rootGroup->add(inspector);

			//whenever the instpector clears, setup default elements
			InspectController::X().onClear += [this] (ElementGroupPtr inspector) {
				inspector->add(Widgets::LiveValueHistory::make("Application fps [Hz]", [] () {
					return ofGetFrameRate();
				}, true));
				inspector->add(Widgets::Button::make("Save all", [this]() {
					this->saveAll();
				}));
				inspector->add(Widgets::Button::make("Load all", [this]() {
					this->loadAll();
				}));
				inspector->add(Widgets::Spacer::make());
			};
			//
			//--




			//--
			// NODE VIEW CALLBACKS
			//--
			//
			for (auto node : *this) {
				auto nodeView = node->getView();
				if (nodeView) {
					//if we click inside the panel, (regardless of what takes the click), then inspect this node

					//we add the listener to be LATE (remember that the mouse stack is notified in reverse)
					//this ensures that anything nested is called first
					nodeView->onMouse.addListener([node, this](MouseArguments & mouse) {
						if (mouse.action == ofxCvGui::MouseArguments::Action::Pressed) {
							ofxCvGui::inspect(node);
						}
					}, -100, this);

					//draw outlines on gui panels if node is selected
					nodeView->onDraw += [node](DrawArguments & drawArgs) {
						if (isBeingInspected(*node)) {
							ofPushStyle();
							ofSetColor(255);
							ofSetLineWidth(3.0f);
							ofNoFill();
							ofRect(drawArgs.localBounds);
							ofPopStyle();
						}
					};

					nodeView->setCaption(node->getName());
				}
			}
			//
			//--


			World::gui = & controller;
			
			if (!this->empty()) {
				ofxCvGui::inspect(this->front());
			}
		}

		//-----------
		void World::saveAll() const {
			for(auto node : * this) {
				node->save(node->getDefaultFilename());
			}
		}

		//-----------
		void World::loadAll(bool printDebug) {
			for(auto node : * this) {
				if (printDebug) {
					ofLogNotice("ofxDigitalEmulsion") << "Loading node [" << node->getName() << "]";
				}
				node->load(node->getDefaultFilename());
			}
		}

		//-----------
		ofxCvGui::Controller & World::getGuiController() {
			if (World::gui) {
				return * World::gui;
			} else {
				throw(Utils::Exception("No gui attached yet"));
			}
		}

		//----------
		ofxCvGui::PanelGroupPtr World::getGuiGrid() const {
			return this->guiGrid;
		}
	}
}
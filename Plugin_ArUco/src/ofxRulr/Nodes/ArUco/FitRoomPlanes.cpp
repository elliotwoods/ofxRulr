#include "pch_Plugin_ArUco.h"
#include "FitRoomPlanes.h"

#include "MarkerMap.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
#pragma mark Capture
			//----------
			FitRoomPlanes::Capture::Capture() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			std::string FitRoomPlanes::Capture::getDisplayString() const {
				return "";
			}

			//----------
			ofxCvGui::ElementPtr FitRoomPlanes::Capture::getDataDisplay() {
				auto element = ofxCvGui::makeElement();
				auto selectMarker = make_shared<ofxCvGui::Widgets::EditableValue<int>>(this->markerIndex);
				{
					element->addChild(selectMarker);
				}

				auto selectPlane = make_shared<ofxCvGui::Widgets::MultipleChoice>("Plane");
				{
					selectPlane->addOptions({ "X", "Y", "Z" });
					selectPlane->entangle(this->plane);
					element->addChild(selectPlane);
				}
				element->onBoundsChange += [this, selectMarker, selectPlane](ofxCvGui::BoundsChangeArguments & args) {
					auto bounds = args.localBounds;
					bounds.height /= 2.0f;
					selectMarker->setBounds(bounds);
					bounds.y += bounds.height;
					selectPlane->setBounds(bounds);
				};
				element->setHeight(80.0f);

				return element;
			}

			//----------
			void FitRoomPlanes::Capture::serialize(Json::Value & json) {
				json << this->markerIndex;
				json << this->plane;
			}

			//----------
			void FitRoomPlanes::Capture::deserialize(const Json::Value & json) {
				json >> this->markerIndex;
				json >> this->plane;
			}

#pragma mark FitRoomPlanes
			//----------
			FitRoomPlanes::FitRoomPlanes() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			std::string FitRoomPlanes::getTypeName() const {
				return "ArUco::FitRoomPlanes";
			}

			//----------
			void FitRoomPlanes::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<MarkerMap>();
			}

			//----------
			void FitRoomPlanes::update() {

			}

			//----------
			void FitRoomPlanes::drawWorldStage() {

			}

			//----------
			void FitRoomPlanes::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addTitle("Captures", ofxCvGui::Widgets::Title::H2);
				this->captureSet.populateWidgets(inspector);

				auto addButton = inspector->addButton("Add...", [this]() {
					auto markerStringComposite = ofSystemTextBoxDialog("Marker index");
					auto markertIndexStrings = ofSplitString(markerStringComposite, ",");
					for (const auto & markerString : markertIndexStrings) {
						if (!markerString.empty()) {
							auto index = ofToInt(markerString);
							auto capture = make_shared<Capture>();
							this->captureSet.add(capture);
							capture->markerIndex = index;
						}
					}
				});
			}

			//----------
			void FitRoomPlanes::serialize(Json::Value & json) {
				this->captureSet.serialize(json["captureSet"]);
			}

			//----------
			void FitRoomPlanes::deserialize(const Json::Value & json) {
				this->captureSet.deserialize(json["captureSet"]);
			}
		}
	}
}
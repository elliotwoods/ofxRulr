#include "pch_Plugin_ArUco.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
#pragma mark Markers::Marker
			//---------
			Markers::Marker::Marker() {
				Utils::AbstractCaptureSet::BaseCapture::onDeserialize += [this](const nlohmann::json& json) {
					this->deserialize(json);
				};
				Utils::AbstractCaptureSet::BaseCapture::onSerialize += [this](nlohmann::json& json) {
					this->serialize(json);
				};

				this->rigidBody = make_shared<Item::RigidBody>();
				this->rigidBody->init();
				this->rigidBody->setName("Marker");
			}

			//---------
			string Markers::Marker::getDisplayString() const {
				return ofToString(this->parameters.ID.get());
			}

			//---------
			string Markers::Marker::getTypeName() const {
				return "MarkerMap::Markers::Marker";
			}

			//---------
			void Markers::Marker::setParent(Markers* markers) {
				this->markers = markers;
			}

			//---------
			void Markers::Marker::drawWorld() {
				if (this->rigidBody->isBeingInspected()) {
					this->rigidBody->drawWorldStage();
				}
			}

			//---------
			void Markers::Marker::serialize(nlohmann::json& json) {
				Utils::serialize(json, "parameters", this->parameters);
				this->rigidBody->serialize(json["rigidBody"]);
			}

			//---------
			void Markers::Marker::deserialize(const nlohmann::json& json) {
				Utils::deserialize(json, "parameters", this->parameters);
				if (json.contains("rigidBody")) {
					this->rigidBody->deserialize(json["rigidBody"]);
				}
			}

			//---------
			vector<glm::vec3> Markers::Marker::getObjectVertices() const {
				return {
					glm::vec3(0, 0, 0)
					, glm::vec3(1, 0, 0) * this->parameters.length.get()
					, glm::vec3(1, 1, 0) * this->parameters.length.get()
					, glm::vec3(0, 1, 0) * this->parameters.length.get()
				};
			}

			//---------
			vector<glm::vec3> Markers::Marker::getWorldVertices() const {
				auto objectVertices = this->getObjectVertices();
				vector<glm::vec3> worldVertices;
				auto transform = this->rigidBody->getTransform();
				for (const auto& objectVertex : objectVertices) {
					worldVertices.push_back(Utils::applyTransform(transform, objectVertex));
				}
				return worldVertices;
			}

			//---------
			ofxCvGui::ElementPtr Markers::Marker::getDataDisplay() {
				// ID
				// Select / inspect
				// Position
				auto elementGroup = ofxCvGui::makeElementGroup();
				auto y = 0;

				vector<shared_ptr<ofxCvGui::Element>> widgets;

				// Top row group
				{
					auto group = ofxCvGui::makeElementGroup();
					vector<shared_ptr<ofxCvGui::Element>> groupWidgets;

					// Marker icon
					{
						auto widget = ofxCvGui::makeElement();
						widget->onDraw += [this](ofxCvGui::DrawArguments& args) {
							if (this->markers) {
								auto detector = this->markers->getInput<ArUco::Detector>();
								if (detector) {
									auto size = min(args.localBounds.getHeight(), args.localBounds.getWidth());
									const auto& markerImage = detector->getMarkerImage(this->parameters.ID.get());
									markerImage.draw(args.localBounds.getCenter() - glm::vec2(size, size) / 2.0f
										, size
										, size);
								}
							}
						};
						group->add(widget);
						groupWidgets.push_back(widget);
					}

					// Locked
					{
						auto widget = make_shared<ofxCvGui::Widgets::Toggle>(this->parameters.fixed);
						widget->setCaption("");
						widget->onDraw += [this](ofxCvGui::DrawArguments& args) {
							const auto& fixed = this->parameters.fixed.get();
							auto size = min(args.localBounds.getHeight(), args.localBounds.getWidth()) * 0.9;
							const auto& image = fixed
								? ofxAssets::image("ofxRulr::lock")
								: ofxAssets::image("ofxRulr::unlock");
							image.draw(args.localBounds.getCenter() - glm::vec2(size, size) / 2.0f
								, size
								, size);
						};
						group->add(widget);
						groupWidgets.push_back(widget);
					}

					// Rigid Body
					{
						auto widget = make_shared<ofxCvGui::Widgets::Toggle>("", [this]() {
							return this->rigidBody->isBeingInspected();
							}
							, [this](bool select) {
								if (select) {
									ofxCvGui::InspectController::X().inspect(this->rigidBody);
								}
								else {
									ofxCvGui::InspectController::X().clear();
								}
							});
						widget->onDraw += [this](ofxCvGui::DrawArguments& args) {
							auto size = min(args.localBounds.getHeight(), args.localBounds.getWidth()) * 0.9;
							auto& icon = this->rigidBody->getIcon();
							icon.draw(args.localBounds.getCenter() - glm::vec2(size / 2, size / 2), size, size);
						};
						group->add(widget);
						groupWidgets.push_back(widget);
					}

					// Ignore
					{
						auto widget = make_shared<ofxCvGui::Widgets::Toggle>(this->parameters.ignore);
						widget->setCaption("");
						widget->onDraw += [this](ofxCvGui::DrawArguments& args) {
							const auto& fixed = this->parameters.ignore.get();
							auto size = min(args.localBounds.getHeight(), args.localBounds.getWidth()) * 0.9;
							const auto& image = ofxAssets::image("ofxRulr::cross");
							image.draw(args.localBounds.getCenter() - glm::vec2(size, size) / 2.0f
								, size
								, size);
						};
						group->add(widget);
						groupWidgets.push_back(widget);
					}

					group->setHeight(50);
					group->onBoundsChange += [groupWidgets](ofxCvGui::BoundsChangeArguments& args) {
						auto itemWidth = args.localBounds.width / groupWidgets.size();
						auto height = args.localBounds.height;
						auto x = 0;
						for (auto widget : groupWidgets) {
							widget->setBounds(ofRectangle(x, 0, itemWidth - 5, height));
							x += itemWidth;
						}
					};

					y += group->getHeight();
					elementGroup->add(group);
					widgets.push_back(group);
				}

				{
					auto widget = make_shared<ofxCvGui::Widgets::EditableValue<int>>(this->parameters.ID);
					widget->setPosition({ 0, y });
					y += widget->getHeight();
					elementGroup->add(widget);
					widgets.push_back(widget);
				}

				{
					auto widget = make_shared<ofxCvGui::Widgets::LiveValue<glm::vec3>>("Position", [this]() {
						return this->rigidBody->getPosition();
						});
					widget->setPosition({ 0, y });
					y += widget->getHeight();
					elementGroup->add(widget);
					widgets.push_back(widget);
				}

				elementGroup->setHeight(y + 5);
				elementGroup->onBoundsChange += [widgets](ofxCvGui::BoundsChangeArguments& args) {
					for (auto widget : widgets) {
						widget->setWidth(args.localBounds.width);
					}
				};
				return elementGroup;
			}

#pragma mark Markers
			//---------
			Markers::Markers() {
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string Markers::getTypeName() const {
				return "MarkerMap::Markers";
			}

			//---------
			void Markers::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->manageParameters(this->parameters);

				this->addInput<ArUco::Detector>();

				// When deserialising update the parent of new markers
				this->onDeserialize.addListener([this](const nlohmann::json&) {
					auto markers = this->markers.getAllCaptures();
					for (auto marker : markers) {
						marker->setParent(this);
					}
				}, this, 100);

				// Create the panel
				{
					this->panel = ofxCvGui::Panels::makeWidgets();
					this->markers.populateWidgets(this->panel);
					this->panel->addButton("Sort", [this]() {
						this->sort();
						});
				}
			}

			//---------
			void Markers::update() {

			}

			//---------
			void Markers::drawWorldStage() {
				auto detector = this->getInput<ArUco::Detector>();
				
				ofMesh markerPreview;
				{
					markerPreview.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);
					markerPreview.addTexCoords({
						{ 0, 0 }
						,{ ARUCO_PREVIEW_RESOLUTION, 0 }
						,{ ARUCO_PREVIEW_RESOLUTION, ARUCO_PREVIEW_RESOLUTION }
						,{ 0, ARUCO_PREVIEW_RESOLUTION }
						});
				}

				//cycle through markers and draw them
				auto markers = this->markers.getSelection();
				for (auto & marker : markers) {
					markerPreview.clearVertices();
					markerPreview.addVertices(marker->getWorldVertices());

					if (!marker->parameters.ignore.get()) {
						if (detector) {
							auto& image = detector->getMarkerImage(marker->parameters.ID.get());

							image.bind();
							{
								markerPreview.draw();
							}
							image.unbind();
						}
						else {
							// Just draw as a quad if we don't have detector
							markerPreview.draw();
						}
					}

					switch (this->parameters.draw.labels.get().get()) {
					case WhenDrawOnWorldStage::Selected:
						if (!this->isBeingInspected()) {
							break;
						}
					case WhenDrawOnWorldStage::Always:
						ofxCvGui::Utils::drawTextAnnotation(ofToString(marker->parameters.ID.get())
							, markerPreview.getVertex(0)
							, this->getColor());
						break;
					case WhenDrawOnWorldStage::Never:
					default:
						break;
					}

					switch (this->parameters.draw.outlines.get().get()) {
					case WhenDrawOnWorldStage::Selected:
						if (!this->isBeingInspected()) {
							break;
						}
					case WhenDrawOnWorldStage::Always:
					{
						ofPolyline line;
						line.addVertices(markerPreview.getVertices());
						line.close();
						ofPushStyle();
						{
							ofSetColor(this->getColor());
							line.draw();
						}
						ofPopStyle();
					}
						break;
					case WhenDrawOnWorldStage::Never:
					default:
						break;
					}

					// Let the marker itself draw to world what it likes
					marker->drawWorld();
				}
			}
			
			//---------
			void Markers::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Add...", [this]() {
					try {
						this->add();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
				inspector->addButton("Delete unfixed markers", [this]() {
					try {
						this->deleteUnfixedMarkers();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
			}

			//---------
			void Markers::serialize(nlohmann::json& json) {
				this->markers.serialize(json);
			}

			//---------
			void Markers::deserialize(const nlohmann::json& json) {
				this->markers.deserialize(json);
			}

			//---------
			ofxCvGui::PanelPtr Markers::getPanel() {
				return this->panel;
			}

			//---------
			const ofImage & Markers::getMarkerImage(int ID) const {
				auto detector = this->getInput<ArUco::Detector>();
				if (detector) {
					return detector->getMarkerImage(ID);
				}
				else {
					static ofImage emptyImage;
					return emptyImage;
				}
			}

			//---------
			vector<shared_ptr<Markers::Marker>> Markers::getMarkers() const {
				return this->markers.getSelection();
			}

			//---------
			shared_ptr<Markers::Marker> Markers::getMarkerByID(int ID) const {
				auto markers = this->markers.getSelection();
				for(auto marker : markers) {
					if (marker->parameters.ID.get() == ID) {
						return marker;
					}
				}

				throw(ofxRulr::Exception("Marker (" + ofToString(ID) + ") not found"));
			}

			//---------
			void Markers::add() {
				this->throwIfMissingAConnection<ArUco::Detector>();
				auto detector = this->getInput<ArUco::Detector>();

				auto newIDString = ofSystemTextBoxDialog("ID");
				if (newIDString.empty()) {
					return;
				}

				auto index = ofToInt(newIDString);
				auto marker = make_shared<Marker>();
				marker->parameters.ID.set(index);

				this->add(marker);
			}

			//---------
			void Markers::add(shared_ptr<Markers::Marker> marker) {
				this->throwIfMissingAConnection<ArUco::Detector>();
				auto detector = this->getInput<ArUco::Detector>();

				marker->setParent(this);
				marker->parameters.length.set(detector->getMarkerLength());
				this->markers.add(marker);
			}

			//---------
			void Markers::sort() {
				this->markers.sortBy([](shared_ptr<Marker> marker) {
					return (float)marker->parameters.ID.get();
					});
			}

			//---------
			void Markers::deleteUnfixedMarkers() {
				vector<shared_ptr<Marker>> markersToDelete;
				auto allMarkers = this->markers.getAllCaptures();
				for (auto marker : allMarkers)
				{
					if (!marker->parameters.fixed.get()) {
						markersToDelete.push_back(marker);
					}
				}

				for (auto markerToDelete : markersToDelete) {
					this->markers.remove(markerToDelete);
				}
			}
		}
	}
}
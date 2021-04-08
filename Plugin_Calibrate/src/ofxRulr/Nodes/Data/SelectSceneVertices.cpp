#include "pch_Plugin_Calibrate.h"
#include "SelectSceneVertices.h"

#include "ofxRulr/Nodes/IHasVertices.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			//----------
			SelectSceneVertices::VertexPicker::VertexPicker(SelectSceneVertices * node) {
				this->setHeight(40.0f);
				this->onDraw += [this, node](ofxCvGui::DrawArguments & args) {
					auto & font = ofxAssets::font(ofxCvGui::getDefaultTypeface(), 12);

					ofPushStyle();
					{
						auto color = this->selectedVertex.found ? ofColor(50, 100, 80) : ofColor(100, 50, 80);
						ofSetColor(this->getMouseState() == Element::Dragging
							? color
							: 50);

						//line to vertex
						if(this->getMouseState() == Element::Dragging) {
							auto & scissorManager = ofxCvGui::Utils::ScissorManager::X();
							bool scissorWasEnabled = scissorManager.getScissorEnabled();
							if (scissorWasEnabled) {
								scissorManager.setScissorEnabled(false);
							}

							glm::vec2 targetPosition;
							if (this->selectedVertex.found) {
								targetPosition = this->selectedVertex.screenPosition;
							}
							else {
								targetPosition = glm::vec2(ofGetMouseX(), ofGetMouseY());
							}

							auto sourcePosition = args.globalBounds.getCenter();
							ofxCvGui::Controller::X().drawDelayed([sourcePosition, targetPosition, color]() {
								ofPushStyle();
								{
									ofSetColor(color);
									ofSetLineWidth(3.0f);
									ofDrawLine(sourcePosition, targetPosition);
								}
								ofPopStyle();
							});

							scissorManager.setScissorEnabled(scissorWasEnabled);
						}
						

						//fill
						ofFill();
						const auto radius = 4.0f;
						ofDrawRectRounded(args.localBounds, radius, radius, radius, radius);

						//outline
						if (this->isMouseOver()) {
							ofNoFill();
							ofSetColor(50);
							ofDrawRectRounded(args.localBounds, radius, radius, radius, radius);
						}

						ofSetColor(255);
						string text;
						if (this->isMouseDragging()) {
							if (this->selectedVertex.found) {
								text = ofToString(this->selectedVertex.worldPosition);
							}
							else {
								if (node->getInput<IHasVertices>()) {
									text = "Drag cursor to vertex";
								}
								else {
									text = "No input connected";
								}
							}
						}
						else {
							text = this->caption;
						}
						ofxCvGui::Utils::drawText(text, args.localBounds, false);
					}

					ofPopStyle();
				};

				this->onMouse += [this, node](ofxCvGui::MouseArguments & args) {
					args.takeMousePress(this);
					if (args.isDragging(this)) {
						auto hasVerticesInput = node->getInput<IHasVertices>();
						if (hasVerticesInput) {
							try {
								this->selectedVertex.worldPosition = hasVerticesInput->getVertexCloseToMouse();
								{
									auto worldStage = ofxRulr::Graph::World::X().getWorldStage();
									this->selectedVertex.screenPosition = worldStage->getCamera().worldToScreen(this->selectedVertex.worldPosition, worldStage->getPanel()->getBounds());
								}
								this->selectedVertex.found = true;
								this->onVertexFound(this->selectedVertex.worldPosition);
							}
							catch (...) {
								this->selectedVertex.found = false;
							}
						}
					}
				};
			}

#pragma mark SelectSceneVertices
			//----------
			SelectSceneVertices::SelectSceneVertices() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string SelectSceneVertices::getTypeName() const {
				return "Data::SelectSceneVertices";
			}

			//----------
			void SelectSceneVertices::init() {
				this->addInput<IHasVertices>();

				RULR_NODE_UPDATE_LISTENER;

				this->onDrawWorldStage += [this]() {
					this->drawWorldStage();
				};

				this->onDeserialize += [this](const nlohmann::json &) {
					auto vertices = this->vertices.getAllCaptures();
					for (auto vertex : vertices) {
						vertex->setOwner(static_pointer_cast<IReferenceVertices>(this->shared_from_this()));
					}
				};

				{
					{
						this->selectNewVertex = make_shared<VertexPicker>(this);
						this->selectNewVertex->setCaption("Drag to select vertex");
						this->selectNewVertex->onVertexFound += [this](glm::vec3 & vertexPosition) {
							this->setNewVertexPosition(vertexPosition);
						};
						this->panel->add(this->selectNewVertex);
					}
					{
						this->panel->addButton("Select world cursor", [this]() {
							this->setNewVertexPosition(ofxRulr::Graph::World::X().getWorldStage()->getCamera().getCursorWorld());
						});
					}
					{
						this->panel->addLiveValue<string>("New vertex", [this]() {
							if (this->newVertex) {
								return ofToString(this->newVertex->worldPosition);
							}
							else {
								return string();
							}
						});
					}
					{
						this->addVertexButton = this->panel->addButton("Add vertex", [this]() {
							if (this->newVertex) {
								this->addVertex(this->newVertex);
								this->newVertex.reset();
							}							
						});
						this->addVertexButton->setHeight(100.0f);
					}
				}
			}

			//----------
			void SelectSceneVertices::update() {
				if (this->newVertex) {
					ofColor color(200, 100, 100);
					color.setHue(int(255.0f * ofGetElapsedTimef()) % 255);
					this->newVertex->color = color;
					this->addVertexButton->setEnabled(true);
				}
				else {
					this->addVertexButton->setEnabled(false);
				}
			}

			//----------
			void SelectSceneVertices::drawWorldStage() {
				if (this->newVertex) {
					this->newVertex->drawWorld();
				}
			}

			//----------
			void SelectSceneVertices::setNewVertexPosition(const glm::vec3 & position) {
				if (!this->newVertex) {
					this->newVertex = make_shared<Vertex>();
				}
				this->newVertex->worldPosition = position;
			}
		}
	}
}
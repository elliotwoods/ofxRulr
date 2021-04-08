#include "pch_Plugin_Calibrate.h"
#include "IReferenceVertices.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
#pragma mark Vertex
				//----------
				IReferenceVertices::Vertex::Vertex() {
					this->onSerialize += [this](nlohmann::json & json) {
						Utils::serialize(json, this->worldPosition);
						Utils::serialize(json, this->viewPosition);
					};
					this->onDeserialize += [this](const nlohmann::json & json) {
						Utils::deserialize(json, this->worldPosition);
						Utils::deserialize(json, this->viewPosition);
						this->onChange.notifyListeners();
					};
					this->onChange += [this]() {
						auto owner = this->owner.lock();
						if (owner) {
							owner->onChangeVertex.notifyListeners();
						}
					};
				}

				//----------
				void IReferenceVertices::Vertex::drawWorld(bool selected /*= false*/) {
					ofPushStyle();
					{
						ofPushMatrix();
						{
							ofTranslate(this->worldPosition);
							//inner
							ofSetLineWidth(2.0f);
							if (selected) {
								ofSetColor(ofGetElapsedTimeMillis() % 255);
							}
							else {
								ofSetColor(this->color);
							}
							this->drawObjectLines();

							//outer
							ofSetLineWidth(4.0f);
							ofSetColor(0);
							this->drawObjectLines();
						}
						ofPopMatrix();
					}
					ofPopStyle();
				}

				//----------
				std::string IReferenceVertices::Vertex::getDisplayString() const {
					stringstream ss;
					ss << "World : " << this->worldPosition << endl;
					ss << "View : " << this->viewPosition;
					return ss.str();
				}

				//----------
				void IReferenceVertices::Vertex::setOwner(shared_ptr<IReferenceVertices> owner) {
					this->owner = owner;
				}

				//----------
				void IReferenceVertices::Vertex::drawObjectLines() {
					ofDrawLine(-glm::vec3(0.05f, 0.0f, 0.0f), glm::vec3(0.05f, 0.0f, 0.0f));
					ofDrawLine(-glm::vec3(0.0f, 0.05f, 0.0f), glm::vec3(0.0f, 0.05f, 0.0f));
					ofDrawLine(-glm::vec3(0.0f, 0.0f, 0.05f), glm::vec3(0.0f, 0.0f, 0.05f));
				}

#pragma mark ISelectTargetVertex
				//----------
				IReferenceVertices::IReferenceVertices() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string IReferenceVertices::getTypeName() const {
					return "Procedure::Calibrate::ISelectTargetVertex";
				}

				//----------
				void IReferenceVertices::init() {
					RULR_NODE_DRAW_WORLD_LISTENER;

					this->onSerialize += [this](nlohmann::json & json) {
						this->vertices.serialize(json["vertices"]);
					};
					this->onDeserialize += [this](const nlohmann::json & json) {
						this->vertices.deserialize(json["vertices"]);
					};

					this->panel = ofxCvGui::Panels::makeWidgets();
					this->vertices.populateWidgets(this->panel);
				}

				//----------
				void IReferenceVertices::drawWorldStage() {
					auto vertices = this->vertices.getSelection();
					for (const auto & vertex : vertices) {
						vertex->drawWorld();
					}
				}

				//----------
				vector<shared_ptr<IReferenceVertices::Vertex>> IReferenceVertices::getSelectedVertices() const {
					return this->vertices.getSelection();
				}

				//----------
				shared_ptr<IReferenceVertices::Vertex> IReferenceVertices::getNextVertex(shared_ptr<Vertex> vertex, int direction /*= 1*/) const {
					auto selectedVertices = this->vertices.getSelection();

					//if empty
					if (selectedVertices.empty()) {
						return shared_ptr<Vertex>();
					}

					auto findVertex = std::find(selectedVertices.begin(), selectedVertices.end(), vertex);
					if (!vertex || findVertex == selectedVertices.end()) {
						//couldn't find existing vertex in our set, just return our first one
						return selectedVertices[0];
					}

					if (direction > 0) {
						for (int i = 0; i < direction; i++) {
							findVertex++;
							if (findVertex == selectedVertices.end()) {
								//loop around
								findVertex = selectedVertices.begin();
							}
						}
					}
					else {
						for (int i = 0; i < -direction; i++) {
							if (findVertex == selectedVertices.begin()) {
								//loop around
								findVertex = selectedVertices.end();
							}
							findVertex--;
						}
					}
					return *findVertex;
				}

				//----------
				void IReferenceVertices::addVertex(shared_ptr<Vertex> vertex) {
					vertex->setOwner(static_pointer_cast<IReferenceVertices>(this->shared_from_this()));
					this->vertices.add(vertex);
				}

				//----------
				ofxCvGui::PanelPtr IReferenceVertices::getPanel() {
					return this->panel;
				}
			}
		}
	}
}
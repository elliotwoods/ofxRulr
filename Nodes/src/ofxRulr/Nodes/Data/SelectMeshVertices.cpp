#include "pch_RulrNodes.h"
#include "SelectMeshVertices.h"

#include "ofxRulr/Nodes/Item/Mesh.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			//----------
			SelectMeshVertices::VertexPicker::VertexPicker(SelectMeshVertices * node) {
				this->setHeight(40.0f);
				this->onDraw += [this, node](ofxCvGui::DrawArguments & args) {
					auto & font = ofxAssets::font(ofxCvGui::getDefaultTypeface(), 12);

					ofPushStyle();
					{
						//fill
						ofSetColor(this->getMouseState() == Element::Dragging ? 80 : 50);
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
								if (node->getInput<Item::Mesh>()) {
									text = "Drag cursor to mesh vertex";
								}
								else {
									text = "Drag cursor to position in scene";
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
					if (args.takeMousePress(this)) {

					}
					if (args.isDragging(this)) {
						auto meshInput = node->getInput<Item::Mesh>();
						if (meshInput) {
							try {
								this->selectedVertex.worldPosition = meshInput->getVertexCloseToMouse();
								this->selectedVertex.found = true;
							}
							catch (...) {
								this->selectedVertex.found = false;
							}
						}
						else {
							try {
								this->selectedVertex.worldPosition = ofxRulr::Graph::World::X().getSummary()->getCursorWorld(true);
								this->selectedVertex.found = true;
							}
							catch (...) {
								this->selectedVertex.found = false;
							}
						}
					}
				};
			}

#pragma mark Vertex
			//----------
			SelectMeshVertices::Vertex::Vertex() {
				this->onSerialize += [this](Json::Value & json) {
					Utils::Serializable::serialize(json, this->position);
				};
				this->onDeserialize += [this](const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->position);
				};
			}

			//----------
			void SelectMeshVertices::Vertex::drawWorld() {
				ofPushStyle();
				{
					ofSetColor(this->color);
					ofPushMatrix();
					{
						ofTranslate(this->position);
						ofDrawLine(ofVec3f(-0.1, 0, 0), ofVec3f(0.1, 0, 0));
						ofDrawLine(ofVec3f(0, -0.1, 0), ofVec3f(0, 0.1, 0));
						ofDrawLine(ofVec3f(0, 0, -0.1), ofVec3f(0, 0, 0.1));
					}
					ofPopMatrix();
				}
				ofPopStyle();
			}

			//----------
			std::string SelectMeshVertices::Vertex::getDisplayString() const {
				return ofToString(this->position);
			}

#pragma mark SelectMeshVertices
			//----------
			SelectMeshVertices::SelectMeshVertices() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string SelectMeshVertices::getTypeName() const {
				return "Data::SelectMeshVertices";
			}

			//----------
			void SelectMeshVertices::init() {
				this->addInput<Item::Mesh>();

				RULR_NODE_SERIALIZATION_LISTENERS;
				this->onDrawWorld += [this]() {
					this->drawWorld();
				};

				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					{
						this->selectNewVertex = make_shared<VertexPicker>(this);
						this->selectNewVertex->setCaption("Select scene vertex");
						panel->add(this->selectNewVertex);
					}
					this->pickedVertices.populateWidgets(panel);
					this->panel = panel;
				}
			}

			//----------
			void SelectMeshVertices::serialize(Json::Value & json) {
				this->pickedVertices.serialize(json);
			}

			//----------
			void SelectMeshVertices::deserialize(const Json::Value & json) {
				this->pickedVertices.deserialize(json);
			}

			//----------
			void SelectMeshVertices::drawWorld() {
				auto pickedVertices = this->pickedVertices.getSelection();
				for (const auto & pickedVertex : pickedVertices) {
					pickedVertex->drawWorld();
				}
				if (this->selectNewVertex) {
					if (this->selectNewVertex->selectedVertex.found) {
						ofPushStyle();
						{
							ofSetColor(int(ofGetElapsedTimef() * 1000) % 255);
							ofPushMatrix();
							{
								ofTranslate(this->selectNewVertex->selectedVertex.worldPosition);
								ofDrawLine(ofVec3f(-0.1, 0, 0), ofVec3f(0.1, 0, 0));
								ofDrawLine(ofVec3f(0, -0.1, 0), ofVec3f(0, 0.1, 0));
								ofDrawLine(ofVec3f(0, 0, -0.1), ofVec3f(0, 0, 0.1));
							}
							ofPopMatrix();
						}
						ofPopStyle();
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr SelectMeshVertices::getPanel() {
				return this->panel;
			}
		}
	}
}
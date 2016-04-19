#include "pch_RulrCore.h"
#include "Patch.h"

#include "ofxCvGui/Widgets/Button.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
#pragma mark View
			//----------
			Patch::View::View(Patch & owner) :
				patchInstance(owner) {
				this->canvasElements->onUpdate += [this](ofxCvGui::UpdateArguments & args) {
					auto newLink = this->patchInstance.newLink;
					if (newLink) {
						auto nodeUnderCursor = this->getNodeHostUnderCursor(this->lastCursorPositionInCanvas);
						newLink->setSource(nodeUnderCursor);
						newLink->setCursorPosition(this->lastCursorPositionInCanvas);
					}
				};
				this->canvasElements->onMouse += [this](ofxCvGui::MouseArguments & args) {
					this->lastCursorPositionInCanvas = args.local;
				};
				this->canvasElements->onDraw += [this](ofxCvGui::DrawArguments & args) {
					auto newLink = this->patchInstance.newLink;
					if (newLink) {
						newLink->draw(args);
					}
					auto selection = this->patchInstance.selection.lock();
					if (selection) {
						//draw outline
						ofPushStyle();
						ofSetLineWidth(1.0f);
						ofNoFill();
						ofDrawRectangle(selection->getBounds());
						ofPopStyle();
					}
					if (this->patchInstance.getNodeHosts().empty()) {
						ofxCvGui::Utils::drawText("Double click to add a new node...", args.localBounds, false);
					}
				};

				this->getCanvasElementGroup()->onDraw.addListener([this](ofxCvGui::DrawArguments & args) {
					this->drawGridLines();
				}, this, -1);

				this->onKeyboard += [this](ofxCvGui::KeyboardArguments & args) {
					if (args.action == ofxCvGui::KeyboardArguments::Action::Pressed) {
						if (args.key == OF_KEY_BACKSPACE || args.key == OF_KEY_DEL) {
							this->patchInstance.deleteSelection();
						}
						if (ofGetKeyPressed(OF_KEY_CONTROL)) {
							switch (args.key) {
							case 'x':
								this->patchInstance.cut();
								break; 
							case 'c':
								this->patchInstance.copy();
								break;
							case 'v':
								this->patchInstance.paste();
								break;
							}
						}
					}
				};

				// NODE BROWSER
				this->nodeBrowser = make_shared<NodeBrowser>();
				this->nodeBrowser->onNewNode += [this](shared_ptr<Nodes::Base> & node) {
					this->patchInstance.addNode(node, ofRectangle(this->birthLocation, this->birthLocation + ofVec2f(200, 100)));
				};
				this->canvasElements->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.isDoubleClicked(this)) {
						this->birthLocation = args.local;
						ofxCvGui::Controller::X().setActiveDialogue(this->nodeBrowser);
						this->nodeBrowser->reset();
					}
				};

				auto wasArbTex = ofGetUsingArbTex();
				ofDisableArbTex();
				this->cell = make_shared<ofTexture>();
				this->cell->enableMipmap();
				this->cell->loadData(ofxAssets::image("ofxRulr::cell-100").getPixels());
				this->cell->setTextureWrap(GL_REPEAT, GL_REPEAT);
				this->cell->setTextureMinMagFilter(GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST);
				if (wasArbTex) ofEnableArbTex();
			}
			
			//----------
			void Patch::View::resync() {
				this->canvasElements->clear();
				const auto & nodeHosts = this->patchInstance.getNodeHosts();
				for (const auto & it : nodeHosts) {
					this->canvasElements->add(it.second);
				}

				const auto & linkHosts = this->patchInstance.getLinkHosts();
				for (const auto & it : linkHosts) {
					this->canvasElements->add(it.second);
				}

				auto newLink = this->patchInstance.getNewLink();
				if (newLink) {
					this->canvasElements->add(newLink);
				}
			}

			//----------
			void Patch::View::drawGridLines() {
				ofPushStyle();
				ofSetColor(255);
				
				auto canvasTopLeft = this->canvasExtents.getTopLeft();
				auto canvasBottomRight = this->canvasExtents.getBottomRight();
				auto canvasSpan = canvasBottomRight - canvasTopLeft;

				this->cell->bind();
				{
					auto plane = ofPlanePrimitive(canvasSpan.x, canvasSpan.y, 2, 2);
					const auto texScale = 0.01f;  // cell is 100x100px
					plane.mapTexCoords(canvasTopLeft.x * texScale, canvasTopLeft.y * texScale, canvasBottomRight.x * texScale, canvasBottomRight.y * texScale);
					
					ofPushMatrix();
					{	
						ofTranslate(canvasTopLeft.x + canvasSpan.x * 0.5, -(canvasTopLeft.y - canvasSpan.y * 0.5), 0.0);
						plane.draw();
					}
					ofPopMatrix();
				}
				this->cell->unbind();

				ofSetColor(100);
				ofNoFill();
				ofDrawRectangle(ofRectangle(canvasTopLeft, canvasBottomRight));

				ofPopStyle();
			}

			//----------
			shared_ptr<NodeHost> Patch::View::getNodeHostUnderCursor(const ofVec2f & cursorInCanvas) {
				shared_ptr<NodeHost> nodeUnderCursor;
				for (shared_ptr<Element> element : this->getCanvasElementGroup()->getElements()) {
					auto asNodeHost = dynamic_pointer_cast<NodeHost>(element);
					if (asNodeHost) {
						if (element->getBounds().inside(cursorInCanvas)) {
							nodeUnderCursor = asNodeHost;
						}
					}
				}
				return nodeUnderCursor;
			}

			//----------
			shared_ptr<NodeHost> Patch::View::getNodeHostUnderCursor() {
				return this->getNodeHostUnderCursor(this->lastCursorPositionInCanvas);
			}

			//----------
			const ofxCvGui::PanelPtr Patch::View::findScreen(const ofVec2f & xy, ofRectangle & currentPanelBounds) {
				
				auto nodeUnderCursor = this->getNodeHostUnderCursor();
				if (nodeUnderCursor) {
					return nodeUnderCursor->getNodeInstance()->getPanel(); // also this will return PanelPtr() if no screen available
				}

				return ofxCvGui::PanelPtr();
			}

#pragma mark Patch
			//----------
			Patch::Patch() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Patch::getTypeName() const {
				return "Patch";
			}

			//----------
			void Patch::init() {
				this->view = MAKE(View, *this);

				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
			}

			//----------
			void Patch::serialize(Json::Value & json) {
				map<shared_ptr<Nodes::Base>, NodeHost::Index> reverseNodeMap;

				//Serialize the nodes
				auto & nodesJson = json["Nodes"];
				for (auto & nodeHost : this->nodeHosts) {
					auto & nodeHostJson = nodesJson[ofToString(nodeHost.first)];

					nodeHost.second->serialize(nodeHostJson);

					//serialize the ID seperately (since the nodeHost doesn't know this information)
					nodeHostJson["ID"] = nodeHost.first;

					//add the node to the reverse map (we'll use this when building links in the next section)
					auto node = nodeHost.second->getNodeInstance();
					reverseNodeMap.insert(pair<shared_ptr<Nodes::Base>, NodeHost::Index>(node, nodeHost.first));
				}

				//Serialize the links
				for (auto & nodeJson : nodesJson) {
					auto & inputPinsJson = nodeJson["InputsPins"];

					auto nodeHost = this->nodeHosts[nodeJson["ID"].asInt()];
					auto node = nodeHost->getNodeInstance();

					for (auto & input : node->getInputPins()) {
						auto & inputPinJson = inputPinsJson[input->getName()];
						if (input->isConnected()) {
							auto linkSource = input->getConnectionUntyped();
							auto linkSourceIndex = reverseNodeMap[linkSource];
							inputPinJson["SourceNode"] = linkSourceIndex;
						}
					}
				}

				auto & canvasJson = json["Canvas"];
				canvasJson["Scroll"] << this->view->getScrollPosition();
			}

			//----------
			void Patch::deserialize(const Json::Value & json) {
				this->nodeHosts.clear();
				
				this->insertPatchlet(json, false);

				const auto & canvasJson = json["Canvas"];
				ofVec2f canvasScrollPosiition;
				canvasJson["Scroll"] >> canvasScrollPosiition;
				this->view->setScrollPosition(canvasScrollPosiition);

			}

			//----------
			void Patch::insertPatchlet(const Json::Value & json, bool useNewIDs, ofVec2f offset) {
				bool hasOffset = offset != ofVec2f();
				map<int, int> reassignIDs;

				const auto & nodesJson = json["Nodes"];
				//Deserialise nodes
				for (const auto & nodeJson : nodesJson) {
					auto ID = (NodeHost::Index) nodeJson["ID"].asInt();
					if (useNewIDs) {
						//use a new ID instead, store a reference to what we changed
						auto newID = this->getNextFreeNodeHostIndex();
						reassignIDs.insert(pair<int, int>(ID, newID));
						ID = newID;
					}
					try {
						auto nodeHost = FactoryRegister::X().make(nodeJson);
						if (hasOffset) {
							auto bounds = nodeHost->getBounds();
							bounds.x += offset.x;
							bounds.y += offset.y;
							nodeHost->setBounds(bounds);
						}
						this->addNodeHost(nodeHost, ID);
					}
					RULR_CATCH_ALL_TO_ERROR
				}

				//Deserialise links into the nodes
				for (const auto & nodeJson : nodesJson) {
					auto ID = (NodeHost::Index) nodeJson["ID"].asInt();
					if (useNewIDs) {
						ID = reassignIDs.at(ID);
					}

					//check we successfully created this node before continuing
					if (this->nodeHosts.find(ID) != this->nodeHosts.end()) {
						auto nodeHost = this->getNodeHost(ID);
						if (nodeHost) {
							auto node = nodeHost->getNodeInstance();
							const auto & inputPinsJson = nodeJson["InputsPins"];

							//go through all the input pins
							for (auto & inputPin : node->getInputPins()) {
								const auto & inputPinJson = inputPinsJson[inputPin->getName()];
								if (!inputPinJson.isNull()) { //check this pin has been serialised to a node ID
									auto sourceNodeHostIndex = (NodeHost::Index) inputPinJson["SourceNode"].asInt();
									if (useNewIDs) {
										sourceNodeHostIndex = reassignIDs.at(sourceNodeHostIndex);
									}
									auto sourceNodeHost = this->getNodeHost(sourceNodeHostIndex);

									//check the node index we want to connect to exists in the patch
									if (sourceNodeHost) {
										auto sourceNode = sourceNodeHost->getNodeInstance();

										//make the connection
										inputPin->connect(sourceNode);
									}
								}
							}
						}
					}
				}

				this->rebuildLinkHosts();
				this->view->resync();
			}

			//----------
			ofxCvGui::PanelPtr Patch::getPanel() {
				return this->view;
			}

			//----------
			void Patch::update() {
				//update selection
				this->selection.reset();
				for (auto nodeHost : this->nodeHosts) {
					if (ofxCvGui::isBeingInspected(nodeHost.second->getNodeInstance())) {
						this->selection = nodeHost.second;
						break;
					}
				}
			}

			//----------
			void Patch::drawWorld() {
				for (auto nodeHost : this->nodeHosts) {
					nodeHost.second->getNodeInstance()->drawWorld();
				}
			}

			//----------
			void Patch::rebuildLinkHosts() {
				this->linkHosts.clear();
				for (auto targetNodeHost : this->nodeHosts) {
					auto targetNode = targetNodeHost.second->getNodeInstance();
					for (auto targetPin : targetNode->getInputPins()) {
						if (targetPin->isConnected()) {
							auto sourceNode = targetPin->getConnectionUntyped();
							auto sourceNodeHost = this->findNodeHost(sourceNode);
							if (sourceNodeHost) {
								auto observedLinkHost = make_shared<ObservedLinkHost>(sourceNodeHost, targetNodeHost.second, targetPin);
								auto linkHost = dynamic_pointer_cast<LinkHost>(observedLinkHost);
								this->linkHosts.insert(pair <LinkHost::Index, shared_ptr<LinkHost>>(this->getNextFreeLinkHostIndex(), linkHost));
							}
						}
					}
				}
			}

			//----------
			const Patch::NodeHostSet & Patch::getNodeHosts() const {
				return this->nodeHosts;
			}

			//----------
			const Patch::LinkHostSet & Patch::getLinkHosts() const {
				return this->linkHosts;
			}

			//----------
			shared_ptr<NodeHost> Patch::addNode(NodeHost::Index index, shared_ptr<Nodes::Base> node, const ofRectangle & bounds) {
				auto nodeHost = make_shared<NodeHost>(node);
				if (bounds != ofRectangle()) {
					nodeHost->setBounds(bounds);
				}
				this->addNodeHost(nodeHost, index);
				return nodeHost;
			}

			//----------
			shared_ptr<NodeHost> Patch::addNode(shared_ptr<Nodes::Base> node, const ofRectangle & bounds) {
				return this->addNode(this->getNextFreeNodeHostIndex(), node, bounds);
			}

			//----------
			shared_ptr<NodeHost> Patch::addNewNode(shared_ptr<BaseFactory> factory, const ofRectangle & bounds) {
				auto newNode = factory->makeUntyped();
				newNode->init();
				return this->addNode(newNode, bounds);
			}

			//----------
			void Patch::addNodeHost(shared_ptr<ofxRulr::Graph::Editor::NodeHost> nodeHost, int index) {
				this->nodeHosts.insert(pair<NodeHost::Index, shared_ptr<NodeHost>>(index, nodeHost));
				weak_ptr<NodeHost> nodeHostWeak = nodeHost;
				nodeHost->onBeginMakeConnection += [this, nodeHostWeak](const shared_ptr<AbstractPin> & inputPin) {
					auto nodeHost = nodeHostWeak.lock();
					if (nodeHost) {
						this->callbackBeginMakeConnection(nodeHost, inputPin);
					}
				};
				nodeHost->onReleaseMakeConnection += [this](ofxCvGui::MouseArguments & args) {
					this->callbackReleaseMakeConnection(args);
				};
				nodeHost->onDropInputConnection += [this](const shared_ptr<AbstractPin> &) {
					this->view->resync();
				};
				nodeHost->getNodeInstance()->onAnyInputConnectionChanged += [this]() {
					this->rebuildLinkHosts();
				};
				this->view->resync();
			}
			
			//----------
			void Patch::addNodeHost(shared_ptr<ofxRulr::Graph::Editor::NodeHost> nodeHost) {
				this->addNodeHost(nodeHost, this->getNextFreeNodeHostIndex());
			}
			
			//----------
			void Patch::deleteSelection() {
				auto selection = this->selection.lock();
				for (auto nodeHost : this->nodeHosts) {
					if (nodeHost.second == selection) {
						this->nodeHosts.erase(nodeHost.first);
						break;
					}
				}
				this->rebuildLinkHosts();
				this->view->resync();
			}

			//----------
			void Patch::cut() {
				if (!this->selection.expired()) {
					this->copy();
					this->deleteSelection();
				}
			}

			//----------
			void Patch::copy() {
				auto selection = this->selection.lock();
				if (selection) {
					//get the json
					Json::Value json;
					selection->serialize(json);

					//push to clipboard
					stringstream jsonString;
					Json::StyledStreamWriter styledWriter;
					styledWriter.write(jsonString, json);
					ofxClipboard::copy(jsonString.str());
				}
			}

			//----------
			void Patch::paste() {
				//get the clipboard into json
				const auto clipboardText = ofxClipboard::paste();
				Json::Value json;
				Json::Reader().parse(clipboardText, json);

				//if we got something
				if (json.isObject()) {
					//let's make it
					auto nodeHost = FactoryRegister::X().make(json);

					//and add it to the patch
					this->addNodeHost(nodeHost);

					//and let's offset the bounds in the clipboard for the next paste
					ofRectangle bounds;
					json["Bounds"] >> bounds;
					bounds.x += 20;
					bounds.y += 20;
					json["Bounds"] << bounds;

					//push the updated copy into the clipboard
					stringstream jsonString;
					Json::StyledStreamWriter styledWriter;
					styledWriter.write(jsonString, json);
					ofxClipboard::copy(jsonString.str());
				}
			}

			//----------
			shared_ptr<TemporaryLinkHost> Patch::getNewLink() const {
				return this->newLink;
			}

			//----------
			shared_ptr<NodeHost> Patch::findNodeHost(shared_ptr<Nodes::Base> node) const {
				for (auto nodeHost : this->nodeHosts) {
					if (nodeHost.second->getNodeInstance() == node) {
						return nodeHost.second;
					}
				}
				return shared_ptr<NodeHost>();
			}

			//----------
			shared_ptr<NodeHost> Patch::getNodeHost(NodeHost::Index index) const {
				shared_ptr<NodeHost> nodeHost;
				if (this->nodeHosts.find(index) != this->nodeHosts.end()) {
					nodeHost = this->nodeHosts.at(index);
				}
				return nodeHost; // Returns empty pointer if not available
			}

			//----------
			void Patch::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Button("Duplicate patch down", [this]() {
					Json::Value json;
					this->serialize(json);
					this->insertPatchlet(json, true, this->view->getCanvasExtents().getBottomLeft());
				}));
				inspector->add(new Widgets::Button("Duplicate patch right", [this]() {
					Json::Value json;
					this->serialize(json);
					this->insertPatchlet(json, true, this->view->getCanvasExtents().getTopRight());
				}));
			}

			//----------
			NodeHost::Index Patch::getNextFreeNodeHostIndex() const {
				if (this->nodeHosts.empty()) {
					return 0;
				}
				else {
					return this->nodeHosts.rbegin()->first + 1;
				}
			}

			//----------
			LinkHost::Index Patch::getNextFreeLinkHostIndex() const {
				if (this->linkHosts.empty()) {
					return 0;
				}
				else {
					return this->linkHosts.rbegin()->first + 1;
				}
			}

			//----------
			void Patch::callbackBeginMakeConnection(shared_ptr<NodeHost> targetNodeHost, shared_ptr<AbstractPin> targetPin) {
				this->newLink = make_shared<TemporaryLinkHost>(targetNodeHost, targetPin);
			}

			//----------
			void Patch::callbackReleaseMakeConnection(ofxCvGui::MouseArguments & args) {
				if (args.button == 2) {
					//right click, clear the link
					this->newLink.reset();
					this->view->resync();
				}
				else if (args.button == 0) {
					//left click, try and make the link
					this->newLink->flushConnection(); // this will trigger a notice downstream to rebuild the list of connections

					//clear the temporary link regardless of success
					this->newLink.reset();
					this->view->resync();
				}
			}
		}
	}
}
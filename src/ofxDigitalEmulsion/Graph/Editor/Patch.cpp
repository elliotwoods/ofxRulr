#include "Patch.h"
#include "ofxAssets.h"
#include "ofSystemUtils.h"

#include "ofxCvGui/Utils/Button.h"

namespace ofxDigitalEmulsion {
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
						ofPushStyle();
						ofSetLineWidth(1.0f);
						ofNoFill();
						ofRect(selection->getBounds());
						ofPopStyle();
					}
					if (this->patchInstance.getNodeHosts().empty()) {
						ofxCvGui::Utils::drawText("Double click to add a new node...", args.localBounds, false);
					}
				};

				this->getCanvasElementGroup()->onDraw.addListener([this](ofxCvGui::DrawArguments & args) {
					this->drawGridLines();
				}, -1, this);

				this->onKeyboard += [this](ofxCvGui::KeyboardArguments & args) {
					if (args.key == OF_KEY_BACKSPACE || args.key == OF_KEY_DEL) {
						this->patchInstance.deleteSelection();
					}
				};

				// NODE BROWSER
				this->nodeBrowser = make_shared<NodeBrowser>();
				this->nodeBrowser->disable(); //starts as hidden
				this->nodeBrowser->addListenersToParent(this, true); // nodeBrowser goes on top of all elements (last listener)
				this->nodeBrowser->onNewNode += [this](shared_ptr<Node> & node) {
					this->patchInstance.addNode(node, ofRectangle(this->birthLocation, this->birthLocation + ofVec2f(200, 100)));
					this->nodeBrowser->disable();
				};
				this->canvasElements->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.isDoubleClicked(this)) {
						this->birthLocation = args.local;
						this->nodeBrowser->enable();
						this->nodeBrowser->reset();
					}
				};
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
				ofNoFill();
				ofSetLineWidth(3.0f);
				ofSetColor(80);
				ofRect(this->canvasExtents);

				const int stepMinor = 20;
				const int stepMajor = 100;

				auto canvasTopLeft = this->canvasExtents.getTopLeft();
				auto canvasBottomRight = this->canvasExtents.getBottomRight();
				for (int x = 0; x < canvasBottomRight.x; x += stepMinor) {
					if (x == 0) {
						ofSetLineWidth(3.0f);
					}
					else if (x % stepMajor == 0) {
						ofSetLineWidth(2.0f);
					}
					else {
						ofSetLineWidth(1.0f);
					}
					ofLine(x, canvasTopLeft.y, x, canvasBottomRight.y);
				}
				for (int x = 0; x > canvasTopLeft.x; x -= stepMinor) {
					if (x == 0) {
						ofSetLineWidth(3.0f);
					}
					else if ((-x) % stepMajor == 0) {
						ofSetLineWidth(2.0f);
					}
					else {
						ofSetLineWidth(1.0f);
					}
					ofLine(x, canvasTopLeft.y, x, canvasBottomRight.y);
				}
				for (int y = 0; y < canvasBottomRight.y; y += stepMinor) {
					if (y == 0) {
						ofSetLineWidth(3.0f);
					}
					else if (y % stepMajor == 0) {
						ofSetLineWidth(2.0f);
					}
					else {
						ofSetLineWidth(1.0f);
					}
					ofLine(canvasTopLeft.x, y, canvasBottomRight.x, y);
				}
				for (int y = 0; y > canvasTopLeft.y; y -= 10) {
					if (y == 0) {
						ofSetLineWidth(3.0f);
					}
					else if ((-y) % stepMajor == 0) {
						ofSetLineWidth(2.0f);
					}
					else {
						ofSetLineWidth(1.0f);
					}
					ofLine(canvasTopLeft.x, y, canvasBottomRight.x, y);
				}

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

#pragma mark Instance
			//----------
			string Patch::getTypeName() const {
				return "Patch";
			}

			//----------
			void Patch::init() {
				this->view = MAKE(View, *this);
			}

			//----------
			void Patch::serialize(Json::Value & json) {
				map<shared_ptr<Node>, NodeHost::Index> reverseNodeMap;

				//Serialize the nodes
				auto & nodesJson = json["Nodes"];
				for (auto & nodeHost : this->nodeHosts) {
					auto & nodeHostJson = nodesJson[ofToString(nodeHost.first)];

					//serialise ID and bounds
					nodeHostJson["ID"] = nodeHost.first;
					nodeHostJson["Bounds"] << nodeHost.second->getBounds();

					//seriaise type name and content
					auto node = nodeHost.second->getNodeInstance();
					nodeHostJson["NodeTypeName"] = node->getTypeName();
					node->serialize(nodeHostJson["Content"]);

					//add the node to the reverse map (we'll use this when building links in the next section)
					reverseNodeMap.insert(pair<shared_ptr<Node>, NodeHost::Index>(node, nodeHost.first));
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
				
				const auto & nodesJson = json["Nodes"];

				//Deserialise nodes
				for (const auto & nodeJson : nodesJson) {
					const auto nodeTypeName = nodeJson["NodeTypeName"].asString();
					const auto ID = (NodeHost::Index) nodeJson["ID"].asInt();

					auto factory = FactoryRegister::X().get(nodeTypeName);
					if (factory) {
						auto node = factory->make();
						node->deserialize(nodeJson["Content"]);

						ofRectangle bounds;
						nodeJson["Bounds"] >> bounds;

						auto nodeHost = this->addNode(ID, node, bounds);
					}
					else {
						OFXDIGITALEMULSION_ERROR << "Failed to load Node #" << ID << " [" << nodeTypeName << "]. No matching Factory found.";
					}
				}

				//Deserialise links into the nodes
				for (const auto & nodeJson : nodesJson) {
					const auto ID = (NodeHost::Index) nodeJson["ID"].asInt();

					//check we successfully created this node before continuing
					if (this->nodeHosts.find(ID) != this->nodeHosts.end()) {
						auto nodeHost = this->getNodeHost(nodeJson["ID"].asInt());
						if (nodeHost) {
							auto node = nodeHost->getNodeInstance();
							const auto & inputPinsJson = nodeJson["InputsPins"];

							//go through all the input pins
							for (auto & inputPin : node->getInputPins()) {
								const auto & inputPinJson = inputPinsJson[inputPin->getName()];
								if (!inputPinJson.isNull()) { //check this pin has been serialised to a node ID
									const auto sourceNodeHostIndex = (NodeHost::Index) inputPinJson["SourceNode"].asInt();
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

				const auto & canvasJson = json["Canvas"];
				ofVec2f canvasScrollPosiition;
				canvasJson["Scroll"] >> canvasScrollPosiition;
				this->view->setScrollPosition(canvasScrollPosiition);

			}

			//----------
			ofxCvGui::PanelPtr Patch::getView() {
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
			shared_ptr<NodeHost> Patch::addNode(NodeHost::Index index, shared_ptr<Node> node, const ofRectangle & bounds) {
				auto nodeHost = make_shared<NodeHost>(node);
				this->nodeHosts.insert(pair<NodeHost::Index, shared_ptr<NodeHost>>(index, nodeHost));
				weak_ptr<NodeHost> nodeHostWeak = nodeHost;
				nodeHost->onBeginMakeConnection += [this, nodeHostWeak](const shared_ptr<BasePin> & inputPin) {
					auto nodeHost = nodeHostWeak.lock();
					if (nodeHost) {
						this->callbackBeginMakeConnection(nodeHost, inputPin);
					}
				};
				nodeHost->onReleaseMakeConnection += [this](ofxCvGui::MouseArguments & args) {
					this->callbackReleaseMakeConnection(args);
				};
				nodeHost->onDropInputConnection += [this](const shared_ptr<BasePin> &) {
					this->rebuildLinkHosts();
					this->view->resync();
				};
				if (bounds != ofRectangle()) {
					nodeHost->setBounds(bounds);
				}
				this->view->resync();
				return nodeHost;
			}

			//----------
			shared_ptr<NodeHost> Patch::addNode(shared_ptr<Node> node, const ofRectangle & bounds) {
				return this->addNode(this->getNextFreeNodeHostIndex(), node, bounds);
			}

			//----------
			shared_ptr<NodeHost> Patch::addNewNode(shared_ptr<BaseFactory> factory, const ofRectangle & bounds) {
				return this->addNode( factory->make(), bounds);
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
			shared_ptr<TemporaryLinkHost> Patch::getNewLink() const {
				return this->newLink;
			}

			//----------
			shared_ptr<NodeHost> Patch::findNodeHost(shared_ptr<Node> node) const {
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
			void Patch::populateInspector2(ofxCvGui::ElementGroupPtr) {

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
			void Patch::callbackBeginMakeConnection(shared_ptr<NodeHost> targetNodeHost, shared_ptr<BasePin> targetPin) {
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
					if (this->newLink->flushConnection()) {
						//if the new link was created, let's rebuild the patch's list of links
						this->rebuildLinkHosts();
					}

					//clear the temporary link regardless of success
					this->newLink.reset();
					this->view->resync();
				}
			}
		}
	}
}
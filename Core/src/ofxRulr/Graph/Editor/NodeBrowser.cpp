#include "pch_RulrCore.h"
#include "NodeBrowser.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
#pragma mark ListItem
			//----------
			NodeBrowser::ListItem::ListItem(shared_ptr<BaseFactory> factory) {
				this->setBounds(ofRectangle(0, 0, 300, 48 + 20));
				this->setCachedView(true);
				this->factory = factory;
				this->icon = factory->makeUntyped()->getIcon();

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					//background to hide ofFbo bad text
					ofPushStyle();
					ofSetColor(80);
					ofFill();
					ofDrawRectangle(args.localBounds);
					ofPopStyle();

					this->icon->draw(10, 10, 48, 48);
					ofxAssets::font("ofxCvGui::swisop3", 20).drawString(this->factory->getModuleTypeName(), 78, (args.localBounds.height + 20) / 2);
				};
			}

			//----------
			shared_ptr<BaseFactory> NodeBrowser::ListItem::getFactory() {
				return this->factory;
			}

			//----------
			shared_ptr<Utils::LambdaDrawable> NodeBrowser::ListItem::getIcon() {
				return this->icon;
			}

#pragma mark NodeBrowser
			/*
			Gui listener tree:
				- this
					- textBox
					- listBox
						- listItems
			*/

			//----------
			NodeBrowser::NodeBrowser() {
				this->setCaption("NodeBrowser");

				this->build();
			}

			//----------
			void NodeBrowser::reset() {
				this->textBox->focus();
				this->textBox->clearText();
			}

			//----------
			void NodeBrowser::refreshResults() {
				//build search tokens
				auto searchTokens = ofSplitString(this->textBox->getText(), " ", true);
				for (auto & searchToken : searchTokens) {
					searchToken = ofToLower(searchToken);
				}
				
				this->listBox->clear();

				auto & factoryRegister = FactoryRegister::X();
				for (auto factoryIterator : factoryRegister) {
					//make sure it matches all tokens within search string
					bool matches = true;
					const auto factoryNameLower = ofToLower(factoryIterator.first);
					for (const auto & searchToken : searchTokens) {
						matches &= factoryNameLower.find(searchToken) != string::npos;
					}

					if (matches) {
						//create list items for matching factories
						auto newListItem = make_shared<ListItem>(factoryIterator.second);
						weak_ptr<ListItem> newListItemWeak = newListItem;
						newListItem->onMouse += [this, newListItemWeak](ofxCvGui::MouseArguments & args) {
							auto newListItem = newListItemWeak.lock();
							if (newListItem) {
								if (args.isLocal()) {
									this->currentSelection = newListItem;
								}
							}
						};
						this->listBox->add(newListItem);
					}
				}
				this->listBox->arrange();

				auto & listItems = this->listBox->getElementGroup()->getElements();
				if (listItems.empty()) {
					this->currentSelection.reset();
				}
				else {
					this->currentSelection = dynamic_pointer_cast<ListItem>(listItems.front());
				}
			}

#define LIST_Y (96 + 10 + 10)

			//----------
			void NodeBrowser::build() {
				this->setCaption("Dialog");

				this->onMouse += [this](ofxCvGui::MouseArguments & args) {
					args.takeMousePress(this);
				};

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();

					//line between text box and list
					ofSetLineWidth(1.0f);
					ofSetColor(50);
					ofDrawLine(0, LIST_Y, args.localBounds.width, LIST_Y);
					ofPopStyle();

					//icon if selected
					auto currentSelection = this->currentSelection.lock();
					string message;
					if (currentSelection) {
						auto icon = currentSelection->getIcon();
						if (icon) {
							icon->draw(10, 10, 96, 96);
						}
						message += currentSelection->getFactory()->getModuleTypeName();
					}
					message += "\n" + ofToString(this->listBox->getElementGroup()->getElements().size()) + " nodes found.\n";
					
					ofxAssets::font("ofxCvGui::swisop3", 12).drawString(message, this->textBox->getBounds().x, 10 + 78);
				};

				this->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					ofRectangle textBoxBounds;
					textBoxBounds.height = 80.0f;
					textBoxBounds.x = 10.0f + 96.0f + 10.0f;
					textBoxBounds.width = args.localBounds.width - textBoxBounds.x - 10.0f;
					textBoxBounds.y = 10.0f;
					this->textBox->setBounds(textBoxBounds);

					ofRectangle listBounds;
					listBounds.y = LIST_Y;
					listBounds.x = 0.0f;	
					listBounds.width = args.localBounds.width;
					listBounds.height = args.localBounds.height - LIST_Y;
					this->listBox->setBounds(listBounds);
				};

				this->buildTextBox();
				this->buildListBox();
			}

			//----------
			void NodeBrowser::buildTextBox() {
				this->textBox = make_shared<ofxCvGui::Utils::TextField>();
				this->textBox->addListenersToParent(this);
				this->textBox->setFont(ofxAssets::font("ofxCvGui::swisop3", 36));
				this->textBox->setHintText("Search...");
				this->textBox->onTextChange += [this](string &) {
					this->refreshResults();
				};
				this->textBox->onHitReturn += [this](string &) {
					this->notifyNewNode();
				};
				this->textBox->onKeyboard += [this](ofxCvGui::KeyboardArguments & args) {
					if (args.action == ofxCvGui::KeyboardArguments::Action::Pressed) {
						bool selectionChanged = false;
						auto nodeList = this->listBox->getElementGroup()->getElements();
						auto currentSelection = this->currentSelection.lock();

						//press up/down to move through items
						if (nodeList.size() > 1 && currentSelection) {
							switch (args.key) {
							case OF_KEY_UP:
								if (currentSelection == nodeList.front()) {
									//if we're going up and on the first item
									this->currentSelection = dynamic_pointer_cast<ListItem>(nodeList.back());
									selectionChanged = true;
								}
								else {
									shared_ptr<ListItem> lastItem;
									for (auto item : nodeList) {
										if (currentSelection == item) {
											this->currentSelection = lastItem;
											selectionChanged = true;
											break;
										}
										lastItem = dynamic_pointer_cast<ListItem>(item);
									}
								}
								break;
								
							case OF_KEY_DOWN:
								if (currentSelection == nodeList.back()) {
									//if we're going down and on the last item
									this->currentSelection = dynamic_pointer_cast<ListItem>(nodeList.front());
									selectionChanged = true;
								}
								else {
									bool lastItemWasSelection = false;
									for (auto item : nodeList) {
										if (lastItemWasSelection) {
											this->currentSelection = dynamic_pointer_cast<ListItem>(item);
											selectionChanged = true;
											break;
										}
										if (currentSelection == item) {
											lastItemWasSelection = true;
										}
									}
								}
								break;
							default:
								break;
							}
						}

						//scroll to the item
						if (selectionChanged) {
							//update the pointer
							currentSelection = this->currentSelection.lock();
							if (currentSelection) {
								this->listBox->scrollToInclude(currentSelection);
							}
						}
					}
				};
			}

			//----------
			void NodeBrowser::buildListBox() {
				this->listBox = make_shared<ofxCvGui::Panels::Scroll>();

				this->listBox->getElementGroup()->onDraw += [this](ofxCvGui::DrawArguments & args) {
					auto currentSelection = this->currentSelection.lock();
					if (currentSelection) {
						//draw background on selected node
						ofPushStyle();
						ofFill();
						ofSetColor(255, 20);
						ofDrawRectangle(currentSelection->getBounds());
						ofPopStyle();
					}
				};
				this->listBox->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.action == ofxCvGui::MouseArguments::Action::Pressed) {
						this->mouseDownInListPosition = args.local;
					}
				};
				this->listBox->onMouseReleased += [this] (ofxCvGui::MouseArguments & args) {
					if (glm::length2(args.local - this->mouseDownInListPosition) < 5 * 5) {
						this->notifyNewNode();
					}
				};
				this->listBox->addListenersToParent(this);
			}

			//----------
			void NodeBrowser::notifyNewNode() {
				auto currentSelection = this->currentSelection.lock();
				if (currentSelection) {
					auto newNode = currentSelection->getFactory()->makeUntyped();
					newNode->init();
					newNode->deserialize(nlohmann::json()); // always call deserialize
					this->onNewNode(newNode);
					ofxCvGui::closeDialog(this);
				}
			}
		}
	}
}
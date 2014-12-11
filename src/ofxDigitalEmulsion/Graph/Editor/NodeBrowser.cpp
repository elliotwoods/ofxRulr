#include "NodeBrowser.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
#pragma mark ListItem
			//----------
			NodeBrowser::ListItem::ListItem(shared_ptr<BaseFactory> factory) {
				this->setBounds(ofRectangle(0, 0, 300, 48 + 20));
				this->setCachedView(true);
				this->factory = factory;

				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					//background to hide ofFbo bad text
					ofPushStyle();
					ofSetColor(80);
					ofFill();
					ofRect(args.localBounds);
					ofPopStyle();

					this->factory->getIcon().draw(10, 10, 48, 48);
					ofxAssets::font("ofxCvGui::swisop3", 20).drawString(this->factory->getNodeTypeName(), 78, (args.localBounds.height + 20) / 2);
				};
			}

			//----------
			shared_ptr<BaseFactory> NodeBrowser::ListItem::getFactory() {
				return this->factory;
			}

#pragma mark NodeBrowser
			//----------
			NodeBrowser::NodeBrowser() {
				this->setCaption("NodeBrowser");

				this->buildBackground();
				this->buildDialog();

				this->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					const float padding = 80.0f;
					auto bounds = args.localBounds;
					bounds.width -= padding * 2;
					bounds.x += padding;
					bounds.height -= padding * 2;
					bounds.y += padding;

					//if too small, let the dialog take the whole window
					if (bounds.width < 100 || bounds.height < 100) {
						bounds = args.localBounds;
					}
					this->dialog->setBounds(bounds);
				};
			}

			//----------
			void NodeBrowser::reset() {
				this->textBox->focus();
				this->textBox->clearText();
			}

			//----------
			void NodeBrowser::refreshResults() {
				string searchTerm = ofToLower(this->textBox->getText());
				
				this->listBox->clear();

				auto & factoryRegister = FactoryRegister::X();
				for (auto factoryIterator : factoryRegister) {
					bool matches = searchTerm == "";
					if (!matches) {
						const auto & factoryName = ofToLower(factoryIterator.first);
						matches |= factoryName.find(searchTerm) != string::npos;
					}
					if (matches) {
						auto newListItem = make_shared<ListItem>(factoryIterator.second);
						newListItem->onMouse += [this, newListItem](ofxCvGui::MouseArguments & args) {
							if (args.isLocal()) {
								this->currentSelection = newListItem;
							}
						};
						this->listBox->add(newListItem);
					}
				}
				this->listBox->arrange();

				auto & listItems = this->listBox->getGroup()->getElements();
				if (listItems.empty()) {
					this->currentSelection.reset();
				}
				else {
					this->currentSelection = dynamic_pointer_cast<ListItem>(listItems.front());
				}
			}

			//----------
			void NodeBrowser::buildBackground() {
				this->background = make_shared<ofxCvGui::Element>();
				this->background->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetColor(0, 200);
					ofRect(args.localBounds);
					ofPopStyle();
				};
				this->background->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.takeMousePress(this->background.get())) {
						this->disable();
					}
				};
				this->background->addListenersToParent(this, true);
			}

#define LIST_Y (96 + 10 + 10)

			//----------
			void NodeBrowser::buildDialog() {
				this->dialog = make_shared<ofxCvGui::ElementGroup>();
				this->dialog->setCaption("Dialog");
				this->dialog->addListenersToParent(this);

				this->dialog->onMouse += [this](ofxCvGui::MouseArguments & args) {
					args.takeMousePress(this->dialog);
				};

				this->dialog->onDraw += [this](ofxCvGui::DrawArguments & args) {
					ofPushStyle();

					//shadow for dialog
					ofFill();
					ofSetColor(0, 100);
					ofPushMatrix();
					ofTranslate(5, 5);
					ofRect(args.localBounds);
					ofPopMatrix();

					//background for dialog
					ofSetColor(80);
					ofRect(args.localBounds);

					//line between text box and list
					ofSetLineWidth(1.0f);
					ofSetColor(50);
					ofLine(0, LIST_Y, args.localBounds.width, LIST_Y);
					ofPopStyle();

					//icon if selected
					auto currentSelection = this->currentSelection.lock();
					string message;
					if (currentSelection) {
						currentSelection->getFactory()->getIcon().draw(10, 10, 96, 96);
						message += currentSelection->getFactory()->getNodeTypeName();
					}
					message += "\n" + ofToString(this->listBox->getGroup()->getElements().size()) + " nodes found.\n";
					
					ofxAssets::font("ofxCvGui::swisop3", 12).drawString(message, this->textBox->getBounds().x, 10 + 78);
				};

				this->dialog->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
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
				this->textBox->addListenersToParent(this->dialog);
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
						auto nodeList = this->listBox->getGroup()->getElements();
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

				this->listBox->getGroup()->onDraw += [this](ofxCvGui::DrawArguments & args) {
					auto currentSelection = this->currentSelection.lock();
					if (currentSelection) {
						//draw background on selected node
						ofPushStyle();
						ofFill();
						ofSetColor(255, 20);
						ofRect(currentSelection->getBounds());
						ofPopStyle();
					}
				};
				this->listBox->onMouse += [this](ofxCvGui::MouseArguments & args) {
					if (args.action == ofxCvGui::MouseArguments::Action::Pressed) {
						this->mouseDownInListPosition = args.local;
					}
				};
				this->listBox->onMouseReleased += [this] (ofxCvGui::MouseArguments & args) {
					if ((args.local - this->mouseDownInListPosition).lengthSquared() < 5 * 5) {
						this->notifyNewNode();
					}
				};
				this->listBox->addListenersToParent(this->dialog);
			}

			//----------
			void NodeBrowser::notifyNewNode() {
				auto currentSelection = this->currentSelection.lock();
				if (currentSelection) {
					auto newNode = currentSelection->getFactory()->make();
					this->onNewNode(newNode);
				}
			}
		}
	}
}
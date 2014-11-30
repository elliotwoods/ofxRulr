#include "NodeBrowser.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
#pragma mark ListItem
			//----------
			NodeBrowser::ListItem::ListItem(shared_ptr<BaseFactory> factory) {
				this->setBounds(ofRectangle(0, 0, 300, 64 + 20));
				auto tempNode = factory->make();
				
				ofImage * icon = &tempNode->getIcon();
				this->setCaption(tempNode->getTypeName());

				this->onDraw += [icon, this](ofxCvGui::DrawArguments & args) {
					icon->draw(10, 10, 64, 64);
					ofxCvGui::Utils::drawText(this->getCaption(), ofRectangle(10 + 64 + 10, 10, args.localBounds.width - 20 - 64 - 10, args.localBounds.height - 20), false);
				};
			}

#pragma mark NodeBrowser
			//----------
			NodeBrowser::NodeBrowser() {
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
			void NodeBrowser::setBirthLocation(const ofVec2f & birthLocation) {
				this->birthLocation = birthLocation;
			}

			//----------
			void NodeBrowser::reset() {
				this->textBox->focus();
				this->textBox->clearText();
			}

			//----------
			void NodeBrowser::refreshResults() {
				string searchTerm = this->textBox->getText();
				
				this->listBox->clear();

				auto & factoryRegister = FactoryRegister::X();
				for (auto factoryIterator : factoryRegister) {
					bool matches = searchTerm == "";
					if (!matches) {
						const auto & factoryName = factoryIterator.first;
						matches |= factoryName.find(searchTerm) != string::npos;
					}
					if (matches) {
						this->listBox->add(make_shared<ListItem>(factoryIterator.second));
					}
				}
				this->listBox->arrange();
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

			//----------
			void NodeBrowser::buildDialog() {
				this->dialog = make_shared<ofxCvGui::ElementGroup>();
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
					ofLine(0, 100.0f, args.localBounds.width, 100.0f);
					ofPopStyle();
				};

				this->dialog->onBoundsChange += [this](ofxCvGui::BoundsChangeArguments & args) {
					ofRectangle textBoxBounds;
					textBoxBounds.height = 80.0f;
					textBoxBounds.width = args.localBounds.width - (20.0f + 64.0f + 10.0f);
					textBoxBounds.y = 10.0f;
					textBoxBounds.x = 10 + 32.0f + 10.0f;
					this->textBox->setBounds(textBoxBounds);

					auto listBounds = args.localBounds;
					listBounds.y = 110.0f;
					listBounds.x = 10.0f;
					listBounds.width -= 20;
					listBounds.height -= listBounds.y + 10;
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
				this->textBox->onTextChange += [this](string &) {
					this->refreshResults();
				};
			}

			//----------
			void NodeBrowser::buildListBox() {
				this->listBox = make_shared<ofxCvGui::Panels::Scroll>();
				this->listBox->addListenersToParent(this->dialog);
			}
		}
	}
}
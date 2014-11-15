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
				auto addButton = std::make_shared<ofxCvGui::Utils::Button>();
				addButton->setCaption("add node");
				addButton->onDrawUp += [](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetColor(255);
					ofxAssets::image("ofxDigitalEmulsion::plus").draw(args.localBounds);
					ofPopStyle();
				};
				addButton->onDrawDown += [](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					ofSetColor(100);
					ofxAssets::image("ofxDigitalEmulsion::plus").draw(args.localBounds);
					ofPopStyle();
				};
				this->fixedElements->add(addButton);
				
				addButton->onMouse += [this, addButton](ofxCvGui::MouseArguments & args) {
					if (args.action == ofxCvGui::MouseArguments::Action::Released) {
						try
						{
							this->patchInstance.addDebug();
						}
						OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
					}
				};

				this->getCanvasElementGroup()->onDraw.addListener([this](ofxCvGui::DrawArguments & args) {
					this->drawGridLines();
				}, -1, this);

				this->onBoundsChange += [this, addButton](ofxCvGui::BoundsChangeArguments & args) {
					addButton->setBounds(ofRectangle(100, 100, 100, 100));
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

			}

			//----------
			void Patch::deserialize(const Json::Value & json) {

			}

			//----------
			ofxCvGui::PanelPtr Patch::getView() {
				return this->view;
			}

			//----------
			void Patch::update() {

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
			void Patch::addNode(NodeHost::Index index, shared_ptr<Node> node) {
				auto nodeHost = make_shared<NodeHost>(node);
				this->nodeHosts.insert(pair<NodeHost::Index, shared_ptr<NodeHost>>(index, nodeHost));
				this->view->resync();
			}

			//----------
			void Patch::addNewNode(shared_ptr<BaseFactory> factory) {
				this->addNode(this->getNextFreeNodeHostIndex(), factory->make());
			}

			//----------
			void Patch::addDebug() {
				auto & factoryRegister = FactoryRegister::X();
				if (factoryRegister.empty()) {
					throw(Utils::Exception("FactoryRegister has no entires"));
				}
				else {
					auto firstFactory = factoryRegister.begin();
					this->addNewNode(firstFactory->second);
				}
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

		}
	}
}
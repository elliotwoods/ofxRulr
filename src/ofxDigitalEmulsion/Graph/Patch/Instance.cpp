#include "Instance.h"
#include "ofxAssets.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Patch {
#pragma mark View
			//----------
			Instance::View::View(Instance & owner) :
				patchInstance(owner) {
				auto addButton = this->getFixedElementGroup()->addBlank();
				addButton->onDraw += [addButton](ofxCvGui::DrawArguments & args) {
					ofPushStyle();
					if (addButton->getMouseState() == ofxCvGui::Element::LocalMouseState::Waiting) {
						ofSetColor(255);
					}
					else {
						ofSetColor(100);
					}
					ofxAssets::image("ofxDigitalEmulsion::plus").draw(args.localBounds);
					ofPopStyle();
				};
				addButton->onMouse += [this, addButton](ofxCvGui::MouseArguments & args) {
					if (args.action == ofxCvGui::MouseArguments::Action::Released) {
						this->patchInstance.addDebug();
					}
				};

				this->getCanvasElementGroup()->onDraw += [this](ofxCvGui::DrawArguments & args) {
					this->drawGridLines();
				};

				this->onBoundsChange += [this, addButton](ofxCvGui::BoundsChangeArguments & args) {
					addButton->setBounds(ofRectangle(100, 100, 100, 100));
				};
			}
			
			//----------
			void Instance::View::drawGridLines() {
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
			string Instance::getTypeName() const {
				return "PatchInstance";
			}

			//----------
			void Instance::init() {
				this->view = MAKE(View, *this);
			}

			//----------
			void Instance::serialize(Json::Value & json) {

			}

			//----------
			void Instance::deserialize(const Json::Value & json) {

			}

			//----------
			ofxCvGui::PanelPtr Instance::getView() {
				return this->view;
			}

			//----------
			void Instance::update() {

			}

			//----------
			void Instance::addDebug() {

				make a new node and host and put it in the patch
			}

			//----------
			void Instance::populateInspector2(ofxCvGui::ElementGroupPtr) {

			}
		}
	}
}
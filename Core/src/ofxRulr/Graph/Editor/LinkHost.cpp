#include "LinkHost.h"
#include "ofPolyline.h"

#include "ofxRulr/Graph/Editor/Patch.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
#pragma mark LinkHost
			//---------
			LinkHost::LinkHost() {
				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					this->callbackDraw(args);
				};
				this->onUpdate += [this](ofxCvGui::UpdateArguments & args) {
					this->updateBounds();
				};
			}

			//---------
			bool LinkHost::isValid() const {
				return !this->sourceNode.expired() && !this->targetNode.expired() && !this->targetPin.expired();
			}

			//---------
			void LinkHost::updateBounds() {
				ofRectangle bounds(this->getSourcePinPosition(), this->getTargetPinPosition());
				bounds.x -= 10;
				bounds.y -= 10;
				bounds.width += 20;
				bounds.height += 20;
				this->setBounds(bounds);
			}
			
			//---------
			ofColor LinkHost::getColor() const {
				auto sourceNode = this->sourceNode.lock();
				if (sourceNode) {
					return sourceNode->getNodeInstance()->getColor();
				}
				else {
					return this->targetPin.lock()->getNodeColor();
				}
			}

			//---------
			void LinkHost::callbackDraw(ofxCvGui::DrawArguments & args) {
				try {
					const ofVec2f sourcePinPosition = this->getSourcePinPosition();
					const ofVec2f targetPinPosition = this->getTargetPinPosition();

					//move from local to patch space
					ofPushMatrix();
					{
						ofTranslate(-this->getBoundsInParent().getTopLeft());

						const auto wireRigidity = ofVec2f(100, 0);
						ofPolyline wire;
						wire.addVertex(sourcePinPosition);
						wire.bezierTo(sourcePinPosition + wireRigidity, targetPinPosition - wireRigidity, targetPinPosition, 40);

						ofPushStyle();
						{

							//shadow
							ofPushMatrix();
							{
								ofTranslate(5.0f, 5.0f);
								ofSetLineWidth(2.0f);
								ofSetColor(0, 100);
								wire.draw();
							}
							ofPopMatrix();

							//outline
							ofSetLineWidth(3.0f);
							ofSetColor(0);
							wire.draw();

							//line
							ofSetLineWidth(2.0f);
							ofSetColor(this->getColor());
							wire.draw();
						}
						ofPopStyle();
					}
					ofPopMatrix();
				}
				catch (ofxRulr::Exception e) {
					ofLogError("ofxRulr::Graph::Editor::LinkHost::draw()") << e.what();
				}
			}

			//---------
			ofVec2f LinkHost::getSourcePinPosition() const {
				auto sourceNode = this->sourceNode.lock();
				if (!sourceNode) {
					throw(ofxRulr::Exception("LinkHost has no valid source node"));;
				}
				return sourceNode->getOutputPinPositionGlobal();
			}

			//---------
			ofVec2f LinkHost::getTargetPinPosition() const {
				auto targetNode = this->targetNode.lock();
				auto targetPin = this->targetPin.lock();
				if (!targetNode || !targetPin) {
					throw(ofxRulr::Exception("LinkHost has no valid target node or pin"));;
				}
				if (targetPin->getIsExposedThroughParentPatch()) {
					// if it'a an exposed  pin, ask the parent patch's node instead
					auto patchNode = (Nodes::Base*) targetPin->getParentPatch();
					return patchNode->getNodeHost()->getInputPinPosition(targetPin);
				}
				else {
					//otherwise ask the node directly
					return targetNode->getInputPinPosition(targetPin);
				}
			}

#pragma mark TemporaryLinkHost
			//----------
			TemporaryLinkHost::TemporaryLinkHost(shared_ptr<NodeHost> targetNode, shared_ptr<AbstractPin> targetPin) {
				this->targetNode = targetNode;
				this->targetPin = targetPin;
			}

			//---------
			void TemporaryLinkHost::setCursorPosition(const ofVec2f & cursorPosition) {
				this->cursorPosition = cursorPosition;
				auto targetNode = this->targetNode.lock();
				if (targetNode) {
					this->setBounds(ofRectangle(cursorPosition, targetNode->getOutputPinPositionGlobal()));
				}
			}

			//---------
			void TemporaryLinkHost::setSource(shared_ptr<NodeHost> source) {
				if (source){
					auto targetPin = this->targetPin.lock();
					if (targetPin->checkSupports(source->getNodeInstance())) {
						this->sourceNode = source;
					}
				} else {
					this->sourceNode.reset();
				}
			}

			//---------
			bool TemporaryLinkHost::flushConnection() {
				if (!this->isValid()) {
					//can't make the connection
					return false;
				}
				auto sourceNode = this->sourceNode.lock();
				auto targetPin = this->targetPin.lock();
				targetPin->connect(sourceNode->getNodeInstance());
				return true;
			}

			//---------
			ofVec2f TemporaryLinkHost::getSourcePinPosition() const {
				if (!this->sourceNode.expired()) {
					auto sourceNode = this->sourceNode.lock();
					return sourceNode->getOutputPinPositionGlobal();
				}
				else {
					return this->cursorPosition;
				}
			}

#pragma mark ObservedLinkHost
			//---------
			ObservedLinkHost::ObservedLinkHost(shared_ptr<NodeHost> sourceNode, shared_ptr<NodeHost> targetNode, shared_ptr<AbstractPin> targetPin) {
				this->sourceNode = sourceNode;
				this->targetNode = targetNode;
				this->targetPin = targetPin;
			}
		}
	}
}
//---------

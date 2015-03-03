#include "LinkHost.h"
#include "ofPolyline.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
#pragma mark LinkHost
			//---------
			LinkHost::LinkHost() {
				this->onDraw += [this](ofxCvGui::DrawArguments & args) {
					this->callbackDraw(args);
				};
			}

			//---------
			bool LinkHost::isValid() const {
				return !this->sourceNode.expired() && !this->targetNode.expired() && !this->targetPin.expired();
			}

			//---------
			void LinkHost::callbackDraw(ofxCvGui::DrawArguments & args) {
				try {
					const ofVec2f sourcePinPosition = this->getSourcePinPosition();
					const ofVec2f targetPinPosition = this->getTargetPinPosition();

					const auto wireRigidity = ofVec2f(100, 0);
					ofPolyline wire;
					wire.addVertex(sourcePinPosition);
					wire.bezierTo(sourcePinPosition + wireRigidity, targetPinPosition - wireRigidity, targetPinPosition, 40);

					//outline
					ofPushStyle();
					ofSetColor(0);
					ofSetLineWidth(4.0f);
					wire.draw();

					ofSetLineWidth(3.0f);
					
					//shadow
					ofPushMatrix();
					ofSetColor(0, 100);
					ofTranslate(5.0f, 5.0f);
					wire.draw();
					ofPopMatrix();

					//line
					ofSetColor(this->targetPin.lock()->getNodeColor());
					wire.draw();

					ofPopStyle();
				}
				catch (ofxDigitalEmulsion::Exception e) {
					ofLogError("ofxDigitalEmulsion::Graph::Editor::LinkHost::draw()") << e.what();
				}
			}

			//---------
			ofVec2f LinkHost::getSourcePinPosition() const {
				auto sourceNode = this->sourceNode.lock();
				if (!sourceNode) {
					throw(ofxDigitalEmulsion::Exception("LinkHost has no valid source node"));;
				}
				return sourceNode->getOutputPinPositionGlobal();
			}

			//---------
			ofVec2f LinkHost::getTargetPinPosition() const {
				auto targetNode = this->targetNode.lock();
				auto targetPin = this->targetPin.lock();
				if (!targetNode || !targetPin) {
					throw(ofxDigitalEmulsion::Exception("LinkHost has no valid target node or pin"));;
				}
				return targetNode->getInputPinPosition(targetPin);
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

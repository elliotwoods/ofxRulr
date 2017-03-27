#include "pch_Plugin_MoCap.h"
#include "OSCRelay.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			//----------
			OSCRelay::OSCRelay() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string OSCRelay::getTypeName() const {
				return "MoCap::OSCRelay";
			}

			//----------
			void OSCRelay::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
			}

			//----------
			void OSCRelay::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void OSCRelay::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
				this->invalidateSender();
			}

			//----------
			void OSCRelay::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addEditableValue<string>(this->parameters.remoteAddress)->onValueChange += [this](const string &) {
					this->invalidateSender();
				};
				inspector->addEditableValue<int>(this->parameters.remotePort)->onValueChange += [this](const int &) {
					this->invalidateSender();
				};
			}

			//----------
			void OSCRelay::processFrame(shared_ptr<UpdateTrackingFrame> incomingFrame) {
				auto sender = this->getSender();
				if (!sender) {
					sender = this->tryMakeSender();
					this->setSender(sender);
				}
				if (!sender) {
					return;
				}

				//send OSC message
				{
					ofxOscMessage message;
					message.setAddress("/transform");
					for (int i = 0; i < 16; i++) {
						message.addFloatArg(incomingFrame->transform.getPtr()[i]);
					}
					sender->sendMessage(message);
				}
				
				this->onNewFrame.notifyListeners(shared_ptr<void*>());
			}

			//----------
			shared_ptr<ofxOscSender> OSCRelay::getSender() const {
				this->senderMutex.lock();
				auto sender = this->sender;
				this->senderMutex.unlock();
				return sender;
			}

			//----------
			shared_ptr<ofxOscSender> OSCRelay::tryMakeSender() const {
				auto sender = make_shared<ofxOscSender>();
				sender->setup(this->parameters.remoteAddress, this->parameters.remotePort);
				return sender;
			}

			//----------
			void OSCRelay::setSender(shared_ptr<ofxOscSender> sender) {
				auto lock = unique_lock<mutex>(this->senderMutex);
				this->sender = sender;
			}

			//----------
			void OSCRelay::invalidateSender() {
				auto lock = unique_lock<mutex>(this->senderMutex);
				this->sender.reset();
			}
		}
	}
}
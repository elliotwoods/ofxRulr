#include "pch_Plugin_Scrap.h"
#include "OSCReceiver.h"
#include "Moon.h"
#include "Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//------------
			OSCReceiver::OSCReceiver()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//------------
			string
				OSCReceiver::getTypeName() const
			{
				return "AnotherMoon::OSCReceiver";
			}

			//------------
			void
				OSCReceiver::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Moon>();
				this->addInput<Lasers>();

				this->manageParameters(this->parameters);
			}

			//------------
			void
				OSCReceiver::update()
			{
				if (this->oscReceiver) {
					if (this->parameters.port.get() != this->oscReceiver->getPort()) {
						this->oscReceiver.reset();
					}

					if (!this->parameters.enabled) {
						this->oscReceiver.reset();
					}
				}

				if (!this->oscReceiver && this->parameters.enabled) {
					this->oscReceiver = make_unique<ofxOscReceiver>();
					this->oscReceiver->setup(this->parameters.port.get());
				}

				if (this->oscReceiver) {
					ofxOscMessage message;

					shared_ptr<Moon> moon;
					auto getMoon = [&]() {
						if (!moon) {
							this->throwIfMissingAConnection<Moon>();
							moon = this->getInput<Moon>();
						}
						return moon;
					};

					vector<shared_ptr<Laser>> lasers;
					auto getLasers = [&]() -> vector<shared_ptr<Laser>>&{
						if (lasers.empty()) {
							this->throwIfMissingAConnection<Lasers>();
							auto lasersNode = this->getInput<Lasers>();
							lasers = lasersNode->getLasersSelected();
						}
						return lasers;
					};

					while (this->oscReceiver->getNextMessage(message)) {
						this->oscFrameNew.notifyFrameNew = true;

						try {
							if (message.getAddress() == "/position" && message.getNumArgs() >= 3) {
								glm::vec3 position{
									message.getArgAsFloat(0)
									, message.getArgAsFloat(1)
									, message.getArgAsFloat(2)
								};
								getMoon()->setPosition(position);
							}

							if (message.getAddress() == "/radius" && message.getNumArgs() >= 1) {
								getMoon()->setDiameter(message.getArgAsFloat(0) * 2.0f);
							}

							if (message.getAddress() == "/diameter" && message.getNumArgs() >= 1) {
								getMoon()->setDiameter(message.getArgAsFloat(0));
							}

							if (message.getAddress() == "/moon" && message.getNumArgs() >= 4) {
								getMoon()->setPosition({
									message.getArgAsFloat(0)
									, message.getArgAsFloat(1)
									, message.getArgAsFloat(2)
									});;

								getMoon()->setDiameter(message.getArgAsFloat(3));
							}

							if (message.getAddress() == "/brightness" && message.getNumArgs() >= 1) {
								auto& lasers = getLasers();
								for (size_t i = 0; i < lasers.size(); i++) {
									auto brightness = ofClamp(message.getArgAsFloat(i % message.getNumArgs()), 0, 1);
									lasers[i]->parameters.deviceState.projection.color.red.set(brightness);
									lasers[i]->parameters.deviceState.projection.color.green.set(brightness);
									lasers[i]->parameters.deviceState.projection.color.blue.set(brightness);
									lasers[i]->pushColor();
								}
							}

							if (message.getAddress() == "/rgb" && message.getNumArgs() >= 1) {
								auto& lasers = getLasers();
								for (size_t i = 0; i < lasers.size(); i++) {
									auto red = ofClamp(message.getArgAsFloat((i * 3 + 0) % message.getNumArgs()), 0, 1);
									auto green = ofClamp(message.getArgAsFloat((i * 3 + 1) % message.getNumArgs()), 0, 1);
									auto blue = ofClamp(message.getArgAsFloat((i * 3 + 2) % message.getNumArgs()), 0, 1);
									lasers[i]->parameters.deviceState.projection.color.red.set(red);
									lasers[i]->parameters.deviceState.projection.color.green.set(green);
									lasers[i]->parameters.deviceState.projection.color.blue.set(blue);
									lasers[i]->pushColor();
								}
							}
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
				}

				{
					this->oscFrameNew.isFrameNew = this->oscFrameNew.notifyFrameNew;
					this->oscFrameNew.notifyFrameNew = false;
				}
			}

			//------------
			void
				OSCReceiver::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addHeartbeat("OSC rx", [this]() {
					return this->oscFrameNew.isFrameNew;
					});
			}
		}
	}
}

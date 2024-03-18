#include "pch_Plugin_Dosirak.h"
#include "OSCReceive.h"
#include "Curves.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			//----------
			OSCReceive::OSCReceive()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				OSCReceive::getTypeName() const
			{
				return "Dosirak::OSCReceive";
			}

			//----------
			void
				OSCReceive::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Curves>();

				this->manageParameters(this->parameters);

				// Setup the panel
				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					panel->addIndicatorBool("Server running", [this]() {
						return (bool)this->oscReceiver;
						});
					panel->addIndicatorBool("Message incoming", [this]() {
						return (bool)this->hasNewMessage;
						});
					this->panel = panel;
				}
			}

			//----------
			void
				OSCReceive::update() {
				// Opening and closing
				this->checkClose();
				this->checkOpen();

				// Receiving messages
				this->hasNewMessage = false;
				if (this->oscReceiver) {
					ofxOscMessage message;
					while (this->oscReceiver->getNextMessage(message)) {
						if (message.getAddress() == this->parameters.address.get()) {
							this->hasNewMessage = true;

							// Collate the messages
							map<string, vector<float>> curveMessages;
							{
								bool hasCurveID = false;
								string curveID;
								for (int i = 0; i < message.getNumArgs(); i++) {
									auto argType = message.getArgType(i);
									if (argType == ofxOscArgType::OFXOSC_TYPE_STRING) {
										// Curve name
										hasCurveID = true;
										curveID = message.getArgAsString(i);

										// Check if already exists and remove it
										if (curveMessages.find(curveID) != curveMessages.end()) {
											curveMessages.erase(curveID);
										}

										// Start a new entry
										curveMessages.emplace(curveID, vector<float>());
									}
									else if (argType == ofxOscArgType::OFXOSC_TYPE_FLOAT
										|| argType == ofxOscArgType::OFXOSC_TYPE_DOUBLE
										|| argType == ofxOscArgType::OFXOSC_TYPE_INT32
										|| argType == ofxOscArgType::OFXOSC_TYPE_INT64)
									{
										// Curve content
										curveMessages[curveID].push_back(message.getArgAsFloat(i));
									}
								}
							}

							// Parse messages into curves
							Data::Dosirak::Curves curves;
							for (const auto& curveMessage : curveMessages) {
								const auto& curveID = curveMessage.first;
								const auto& content = curveMessage.second;
								if (content.size() < 3) {
									// ignore this curve
									continue;
								}

								Data::Dosirak::Curve curve;
								// Take the color
								curve.color.r = content[0];
								curve.color.g = content[1];
								curve.color.b = content[2];

								auto i = 3;

								// Take the vertices
								// e.g. i=3, c=6
								while (content.size() >= i + 3) {
									glm::vec3 point{
										content[i + 0]
										, content[i + 1]
										, content[i + 2]
									};
									curve.points.push_back(point);
									i += 3;
								}

								curves.emplace(curveID, curve);
							}

							// Store the curves
							auto curvesNode = this->getInput<Curves>();
							if (curvesNode) {
								curvesNode->setCurves(curves);
							}
						}
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr
				OSCReceive::getPanel()
			{
				return this->panel;
			}

			//----------
			void 
				OSCReceive::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;
			}

			//----------
			void
				OSCReceive::checkClose()
			{
				if (!this->oscReceiver) {
					return;
				}

				if (this->parameters.port.get() != this->oscReceiver->getPort()) {
					this->oscReceiver.reset();
					return;
				}

				if (!this->parameters.enabled) {
					this->oscReceiver.reset();
					return;
				}

				if (!this->oscReceiver->isListening()) {
					this->oscReceiver.reset();
					return;
				}
			}

			//----------
			void
				OSCReceive::checkOpen()
			{
				if (this->oscReceiver) {
					return;
				}

				if (this->parameters.enabled) {
					this->oscReceiver = make_unique<ofxOscReceiver>();
					ofxOscReceiverSettings settings;
					{
						settings.port = this->parameters.port.get();
					}
					this->oscReceiver->setup(settings);
				}
			}
		}
	}
}
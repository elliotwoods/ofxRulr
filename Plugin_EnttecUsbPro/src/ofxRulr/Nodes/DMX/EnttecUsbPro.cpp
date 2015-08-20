#include "EnttecUsbPro.h"

#include "ofxCvGui/Widgets/EditableValue.h"

//from smallfly/ofxDmx
#define DMX_PRO_HEADER_SIZE 4
#define DMX_PRO_START_MSG 0x7E
#define DMX_START_CODE 0
#define DMX_START_CODE_SIZE 1
#define DMX_PRO_SEND_PACKET 6 // "periodically send a DMX packet" mode
#define DMX_PRO_END_SIZE 1
#define DMX_PRO_END_MSG 0xE7

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			EnttecUsbPro::EnttecUsbPro() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void EnttecUsbPro::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->portName.set("Port name", "COM1");

				//we connect in deserialise
			}

			//----------
			string EnttecUsbPro::getTypeName() const {
				return "DMX::EnttecUsbPro";
			}

			//----------
			void EnttecUsbPro::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->portName, json);
			}

			//----------
			void EnttecUsbPro::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->portName, json);
				this->connect();
			}

			//----------
			void EnttecUsbPro::connect() {
				this->disconnect();

				try {
					this->sender = make_shared<ofSerial>();
					this->sender->setup(this->portName.get(), 57600);
					if (!this->sender->isInitialized()) {
						throw(Exception("Failed to open port " + this->portName.get()));
					}
				}
				RULR_CATCH_ALL_TO_ALERT;
				if (this->sender && !this->sender->isInitialized()) {
					this->sender.reset();
				}
			}

			//----------
			void EnttecUsbPro::disconnect() {
				if (this->sender) {
					this->sender->close();
					this->sender.reset();
				}
			}

			//----------
			void EnttecUsbPro::sendUniverse(UniverseIndex index, shared_ptr<Universe> universe) {
				if (this->sender && universe) {
					//code taken from ofxDmx

					//we only have one universe, so send it

					ChannelIndex dataSize = 512 + DMX_START_CODE_SIZE;
					unsigned int packetSize = DMX_PRO_HEADER_SIZE + dataSize + DMX_PRO_END_SIZE;
					DMX::Value * packet = new DMX::Value[packetSize];

					// header
					packet[0] = DMX_PRO_START_MSG;
					packet[1] = DMX_PRO_SEND_PACKET;
					packet[2] = dataSize & 0xff; // data length lsb
					packet[3] = (dataSize >> 8) & 0xff; // data length msb

														// data
					packet[4] = DMX_START_CODE; // first data byte
					memcpy(packet + 5, universe->getChannels() + 1, 512);

					// end
					packet[packetSize - 1] = DMX_PRO_END_MSG;

					this->sender->writeBytes(&packet[0], packetSize);

					delete[] packet;
				}
			}

			//----------
			void EnttecUsbPro::populateInspector(ElementGroupPtr inspector) {
				auto portNameWidget = Widgets::EditableValue<string>::make(this->portName);
				portNameWidget->onEditValue += [this](string & portName) {
					this->connect();
				};
				inspector->add(portNameWidget);
			}
		}
	}
}
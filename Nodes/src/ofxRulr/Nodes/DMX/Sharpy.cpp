#include "Sharpy.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			Sharpy::Sharpy() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Sharpy::init() {
				this->pan.setMin(-270);
				this->pan.setMax(270);

				this->tilt.setMin(-126);
				this->tilt.setMax(126);

				this->channels.push_back(make_shared<Channel>("Colour wheel"));
				this->channels.push_back(make_shared<Channel>("Stop / Strobe"));
				this->channels.push_back(make_shared<Channel>("Dimmer", [this]() { return (DMX::Value) (this->brightness * 255.0f); }));
				this->channels.push_back(make_shared<Channel>("Static gobo change"));
				this->channels.push_back(make_shared<Channel>("Prism insertion"));
				this->channels.push_back(make_shared<Channel>("Prism rotation"));
				this->channels.push_back(make_shared<Channel>("Effects movement"));
				this->channels.push_back(make_shared<Channel>("Frost"));
				this->channels.push_back(make_shared<Channel>("Focus"));
				this->channels.push_back(make_shared<Channel>("Pan", [this]() {
					auto panAll = (int) ofMap(this->pan.get(), this->pan.getMin(), this->pan.getMax(), 0, std::numeric_limits<uint16_t>::max());
					return (DMX::Value) (panAll >> 8);
				}));
				this->channels.push_back(make_shared<Channel>("Pan fine", [this]() {
					auto panAll = (int)ofMap(this->pan.get(), this->pan.getMin(), this->pan.getMax(), 0, std::numeric_limits<uint16_t>::max());
					return (DMX::Value) (panAll % 256);
				}));
				this->channels.push_back(make_shared<Channel>("Tilt", [this]() {
					auto tiltAll = (int)ofMap(this->tilt.get(), this->tilt.getMin(), this->tilt.getMax(), 0, std::numeric_limits<uint16_t>::max());
					return (DMX::Value) (tiltAll >> 8);
				}));
				this->channels.push_back(make_shared<Channel>("Tilt fine", [this]() {
					auto tiltAll = (int)ofMap(this->tilt.get(), this->tilt.getMin(), this->tilt.getMax(), 0, std::numeric_limits<uint16_t>::max());
					return (DMX::Value) (tiltAll % 256);
				}));
				this->channels.push_back(make_shared<Channel>("Function"));
				this->channels.push_back(make_shared<Channel>("Reset"));
				this->channels.push_back(make_shared<Channel>("Lamp Control", [this]() {
					if (this->powerStateSignal) {
						return 255;
					}
					else {
						return 50;
					}
				}));
			}

			//----------
			string Sharpy::getTypeName() const {
				return "DMX::Sharpy";
			}
		}
	}
}

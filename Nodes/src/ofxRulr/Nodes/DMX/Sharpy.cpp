#include "pch_RulrNodes.h"
#include "Sharpy.h"

#include "ofxCvGui/Widgets/Toggle.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			//----------
			Sharpy::Sharpy() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Sharpy::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->pan.setMin(-270);
				this->pan.setMax(270);

				this->tilt.setMin(-126);
				this->tilt.setMax(126);

				this->channels.push_back(make_shared<Channel>("Colour wheel"));
				this->channels.push_back(make_shared<Channel>("Stop / Strobe"));
				this->channels.push_back(make_shared<Channel>("Dimmer", [this]() { return (DMX::Value) (this->brightness * 255.0f); }));
				this->channels.push_back(make_shared<Channel>("Static gobo change", [this]() {
					auto goboSelection = (int) round(ofMap(this->iris.get(), 0.0f, 1.0f, 0, 6, true));
					
					//0 is fully open, 1 is most closed, 6 is almost fully open
					//so swizzle the selection
					if (goboSelection == 6) {
						goboSelection = -1;
					}
					goboSelection++;

					return (float)goboSelection / 6.0f * 27;
				}));
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
				
				//vector channels
				this->vectorChannels.push_back(make_shared<Channel>("Pan - Tilt time"));
				this->vectorChannels.push_back(make_shared<Channel>("Colour time"));
				this->vectorChannels.push_back(make_shared<Channel>("Beam time"));
				this->vectorChannels.push_back(make_shared<Channel>("Gobo time"));
				for (auto vectorChannel : this->vectorChannels) {
					vectorChannel->enabled.set(false);
					this->channels.push_back(vectorChannel);
				}

				this->vectorChannelsEnabled.set("Vector channels enabled", false);

				this->rebootState.rebooting = false;
				this->rebootState.rebootBeginTime = 0.0f;
			}

			//----------
			string Sharpy::getTypeName() const {
				return "DMX::Sharpy";
			}

			//----------
			void Sharpy::update() {
				if (this->rebootState.rebooting) {
					this->getChannel("Reset")->set(255);
					if (ofGetElapsedTimef() - this->rebootState.rebootBeginTime > 10.0f) {
						this->rebootState.rebooting = false;
					}
				}
				else {
					this->getChannel("Reset")->set(0);
				}
			}

			//----------
			void Sharpy::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->vectorChannelsEnabled, json);
			}

			//----------
			void Sharpy::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->vectorChannelsEnabled, json);
				this->updateVectorChannelsEnabled();
			}

			//----------
			void Sharpy::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				auto vectorChannelsEnabledWidget = Widgets::Toggle::make(this->vectorChannelsEnabled);
				vectorChannelsEnabledWidget->onValueChange += [this](ofParameter<bool> &) {
					this->updateVectorChannelsEnabled();
					InspectController::X().refresh();
				};
				inspector->add(vectorChannelsEnabledWidget);
			}

			//----------
			void Sharpy::setColorIndex(int colorIndex) {
				colorIndex = ofClamp(colorIndex, 0, 13);
				auto value = (float)colorIndex * 8.57f;
				value = floor(value);
				this->channels[0]->value.set(value); //steps between colours`
			}

			//----------
			void Sharpy::reboot() {
				this->rebootState.rebooting = true;
				this->rebootState.rebootBeginTime = ofGetElapsedTimef();
			}

			//----------
			void Sharpy::updateVectorChannelsEnabled() {
				for (auto channel : this->vectorChannels) {
					channel->enabled.set(this->vectorChannelsEnabled.get());
				}
			}
		}
	}
}

#include "pch_RulrNodes.h"
#include "Fixture.h"

#include "ofxRulr/Nodes/DMX/Transmit.h"

#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Spacer.h"
#include "ofxCvGui/Widgets/EditableValue.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
#pragma mark Fixture::Channel
			//----------
			Fixture::Channel::Channel() : 
			Channel("Value") {
			}

			//----------
			Fixture::Channel::Channel(string name, DMX::Value defaultValue) {
				this->value.set(name, defaultValue, 0, 255);
				this->enabled.set("Enabled", true);
			}

			//----------
			Fixture::Channel::Channel(string name, function<DMX::Value()> generateValue) : 
			Channel(name) {
				this->generateValue = generateValue;
			}

			//----------
			void Fixture::Channel::set(DMX::Value value) {
				this->value.set(ofClamp(value, 0, 255));
			}

#pragma mark Fixture
			//----------
			Fixture::Fixture() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Fixture::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<DMX::Transmit>();

				this->channelIndex.set("Channel index", 1, 1, 512);
				this->universeIndex.set("Universe index", 0, 0, 1024);
			}

			//----------
			string Fixture::getTypeName() const {
				return "DMX::Fixture";
			}

			//----------
			void Fixture::update() {
				//calculate DMX
				vector<DMX::Value> output;
				for (int i = 0; i < this->channels.size(); i++) {
					auto channel = this->channels[i];
					if (channel->enabled.get() == false) {
						//break on first disabled channel
						break;
					}
					auto & generator = channel->generateValue;
					if (generator) {
						auto value = generator();
						channel->value.set(value);
					}
					output.push_back(this->channels[i]->value);
				}
				
				//transmit DMX
				auto transmit = this->getInput<DMX::Transmit>();
				if (transmit) {
					auto universe = transmit->getUniverse(this->universeIndex.get());
					if (universe) {
						universe->setChannels((DMX::ChannelIndex) this->channelIndex.get()
							, &output[0]
							, (DMX::ChannelIndex) output.size());
					}
					else {
						RULR_ERROR << "Universe index " << this->universeIndex << " is out of bounds for this sender";
					}
				}
			}

			//----------
			void Fixture::serialize(nlohmann::json & json) {
				json << this->channelIndex;
				json << this->universeIndex;
				
				auto & jsonChannels = json["channels"];
				for (auto channel : this->channels) {
					jsonChannels << channel->value;
				}
			}

			//----------
			void Fixture::deserialize(const nlohmann::json & json) {
				json >> this->channelIndex;
				json >> this->universeIndex;

				const auto & jsonChannels = json["channels"];
				for (auto channel : this->channels) {
					jsonChannels >> channel->value;
				}
			}

			//----------
			void Fixture::setChannelIndex(DMX::ChannelIndex channelIndex) {
				this->channelIndex = channelIndex;
			}

			//----------
			const vector<shared_ptr<Fixture::Channel>> & Fixture::getChannels() const {
				return this->channels;
			}

			//----------
			shared_ptr<Fixture::Channel> Fixture::getChannel(DMX::ChannelIndex index) {
				return this->channels[index];
			}

			//----------
			shared_ptr<Fixture::Channel> Fixture::getChannel(string name) {
				for (auto channel : this->channels) {
					if (channel->value.getName() == name) {
						return channel;
					}
				}
				return shared_ptr<Fixture::Channel>();
			}

			//----------
			void Fixture::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Title("DMX::Fixture", Widgets::Title::Level::H2));
				inspector->add(new Widgets::EditableValue<ChannelIndex>(this->channelIndex));
				inspector->add(new Widgets::EditableValue<ChannelIndex>(this->universeIndex));
				
				for (int i = 0; i < this->channels.size(); i++) {
					auto channel = this->channels[i];
					if (channel->enabled.get() == false) {
						//break on first disabled channel.
						break;
					}

					if (i % 8 == 0) {
						auto rangeMin = i + 1;
						auto rangeMax = MIN(i + 8, this->channels.size());
						inspector->add(new Widgets::Spacer());
						inspector->add(new Widgets::Title("Channels " + ofToString(rangeMin) + "..." + ofToString(rangeMax), Widgets::Title::Level::H3));
					}

					if (channel->generateValue) {
						//if it has a value generator, make a live value
						inspector->add(new Widgets::LiveValue<int>(channel->value.getName(), [this, channel]() {
							return channel->value.get();
						}));
					}
					else {
						//make a slider
						auto slider = inspector->add(new Widgets::Slider(this->channels[i]->value));
						slider->addIntValidator();
					}
				}
			}
		}
	}
}
#include "pch_RulrNodes.h"
#include "Channel.h"

#include "ofUtils.h"

namespace ofxRulr {
	namespace Data {
		namespace Channels {
			//----------
			Channel::Channel(const string & name) : name(name) {

			}

			//----------
			const string & Channel::getName() const {
				return this->name;
			}

			//----------
			Channel & Channel::getSubChannel(const string & name) {
				auto findChannel = this->subChannels.find(name);
				if (findChannel == this->subChannels.end()) {
					//channel doesn't exist, create it
					return this->addSubChannel(name);
				}
				else {
					return * findChannel->second;
				}
			}

			//----------
			Channel::Set & Channel::getSubChannels() {
				return this->subChannels;
			}

			//----------
			const Channel::Set & Channel::getSubChannels() const {
				return this->subChannels;
			}

			//----------
			Channel & Channel::operator[](const string & subChannelName) {
				return this->getSubChannel(subChannelName);
			}

			//----------
			Channel & Channel::operator[](const Address & address) {
				if (address.empty()) {
					return *this;
				}
				else {
					auto subAddressFront = address.front();
					auto subAddressMinusFront = address;
					subAddressMinusFront.pop_front();

					auto & channel = this->getSubChannel(subAddressFront);
					if (address.empty()) {
						return channel;
					}
					else {
						return channel[subAddressMinusFront];
					}
				}
			}

			//----------
			shared_ptr<ofAbstractParameter> Channel::getParameterUntyped() {
				return this->parameter;
			}

			//----------
			Channel::Type Channel::getValueType() const {
				return this->type;
			}

			//----------
			Channel & Channel::addSubChannel(const string & name) {
				auto channel = make_shared<Channel>(name);
				pair<string, shared_ptr<Channel>> inserter = {
					name,
					channel
				};
				this->subChannels.insert(inserter);
				channel->onHeirarchyChange += [this]() {
					this->onHeirarchyChange.notifyListeners();
				};
				this->onHeirarchyChange.notifyListeners();

				return *channel;
			}

			//----------
			void Channel::removeSubChannel(const string & name) {
				auto findChannel = this->subChannels.find(name);
				if (findChannel != this->subChannels.end()) {
					this->subChannels.erase(findChannel);
					this->onHeirarchyChange.notifyListeners();
				}
			}

			//----------
			void Channel::setSubChannel(shared_ptr<Channel> channel) {
				this->subChannels[channel->getName()] = channel;

				//we always notify because otherwise we have to fully compare channel with what might have been here before
				this->onHeirarchyChange.notifyListeners();
			}

			//----------
			void Channel::clear() {
				this->parameter.reset();
				this->subChannels.clear();
				this->onHeirarchyChange.notifyListeners();
			}
		}
	}
}
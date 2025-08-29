#include "pch_Plugin_Reworld.h"
#include "Router.h"

#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			bool
				Router::Address::operator<(const Address& other) const
			{
				if (this->column < other.column) {
					return true;
				}
				if (this->column == other.column) {
					return this->portal < other.portal;
				}
				else {
					return false;
				}
			}

			//----------
			Router::Router()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Router::getTypeName() const
			{
				return "Reworld::Router";
			}

			//----------
			void
				Router::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				this->manageParameters(this->parameters);

				this->onPopulateInspector += [this](ofxCvGui::InspectArguments& args) {
					auto inspector = args.inspector;
					inspector->addButton("Test", [this]() {
						try {
							this->test();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, ' ');
					};
			}

			//----------
			void
				Router::update()
			{
				// OSC settings
				{
					if (this->oscSender) {
						if (this->oscSender->getPort() != this->parameters.osc.port.get()) {
							this->oscSender.reset();
						}
						else if (this->oscSender->getHost() != this->parameters.hostname.get()) {
							this->oscSender.reset();
						}
					}
					if (!this->oscSender) {
						this->oscSender = make_shared<ofxOscSender>();
						this->oscSender->setup(this->parameters.hostname.get(), this->parameters.osc.port.get());
					}
				}
			}

			//----------
			string
				Router::getBaseURI() const
			{
				return "http://" + this->parameters.hostname.get() + ":" + ofToString(this->parameters.rest.port.get());
			}

			//----------
			string
				Router::getBaseURI(const Address& address) const
			{
				return this->getBaseURI() + "/" + ofToString(address.column) + "/" + ofToString((int)address.portal);
			}

			//----------
			nlohmann::json
				Router::loadURI(const string& uri)
			{
				auto response = ofLoadURL(uri);

				if (response.status != 200) {
					throw(ofxRulr::Exception(ofToString(response.status) + "\n" + response.error + "\n" + (string)response.data));
				}
				if (response.data.size() == 0) {
					return nlohmann::json();
				}
				else {
					return nlohmann::json::parse((string)response.data);
				}
			}

			//----------
			void
				Router::test()
			{
				auto uri = this->getBaseURI();
				this->loadURI(uri);
			}

			//----------
			void
				Router::setPosition(const Address& address, const glm::vec2& position)
			{
				auto uri = this->getBaseURI(address) + "/setPosition/" + ofToString(position.x) + "," + ofToString(position.y);
				this->loadURI(uri);
			}

			//----------
			glm::vec2
				Router::getPosition(const Address& address)
			{
				auto uri = this->getBaseURI(address) + "/getPosition";
				auto responseJson = this->loadURI(uri);
				if (responseJson.contains("x") && responseJson.contains("y")) {
					return glm::vec2{
						responseJson["x"]
						, responseJson["y"]
					};
				}
				else {
					throw(ofxRulr::Exception("Malformed json response"));
				}
			}

			//----------
			glm::vec2
				Router::getTargetPosition(const Address& address)
			{
				auto uri = this->getBaseURI(address) + "/getTargetPosition";
				auto responseJson = this->loadURI(uri);
				if (responseJson.contains("x") && responseJson.contains("y")) {
					return glm::vec2{
						responseJson["x"]
						, responseJson["y"]
					};
				}
				else {
					throw(ofxRulr::Exception("Malformed json response"));
				}
			}

			//----------
			bool
				Router::isInPosition(const Address& address)
			{
				auto uri = this->getBaseURI(address) + "/isInPosition";
				auto responseJson = this->loadURI(uri);
				return (bool)responseJson;
			}

			//----------
			void
				Router::poll(const Address& address)
			{
				auto uri = this->getBaseURI(address) + "/poll";
				this->loadURI(uri);
			}

			//----------
			void
				Router::push(const Address& address)
			{
				auto uri = this->getBaseURI(address) + "/push";
				this->loadURI(uri);
			}

			//----------
			void
				Router::sendAxisValues(const map<Address, Models::Reworld::AxisAngles<float>>& axisAnglesByIndex)
			{
				if (!this->oscSender) {
					return;
				}
				ofxOscMessage message;
				auto address = "/axesMoveByInidices";
				message.setAddress(address);
				for (const auto& it : axisAnglesByIndex) {
					message.addIntArg(it.first.column);
					message.addIntArg(it.first.portal);
					message.addFloatArg(it.second.A);
					message.addFloatArg(it.second.B);

					// clear out the message if it's getting too big
					if (message.getNumArgs() / 4 >= this->parameters.osc.maxPortalsPerMessage.get()) {
						// send this batch
						this->oscSender->sendMessage(message);

						// reset the message
						message.clear();
						message.setAddress(address);
					}
				}
				
				if (message.getNumArgs() > 0) {
					this->oscSender->sendMessage(message);
				}
			}
		}
	}
}
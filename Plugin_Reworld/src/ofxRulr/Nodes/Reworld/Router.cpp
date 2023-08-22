#include "pch_Plugin_Reworld.h"
#include "Router.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
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
			string
				Router::getBaseURI() const
			{
				return "http://" + this->parameters.hostname.get() + ":" + ofToString(this->parameters.port.get());
			}

			//----------
			string
				Router::getBaseURI(const Address& address) const
			{
				return this->getBaseURI() + "/" + ofToString(address.column) + "/" + ofToString((int) address.portal);
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
		}
	}
}
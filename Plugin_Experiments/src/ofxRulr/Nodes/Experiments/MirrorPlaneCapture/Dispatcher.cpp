#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//----------
				Dispatcher::Dispatcher() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Dispatcher::getTypeName() const {
					return "Halo::Dispatcher";
				}

				//----------
				void Dispatcher::init() {
					this->manageParameters(this->parameters);

					RULR_NODE_INSPECTOR_LISTENER;
				}

				//----------
				void Dispatcher::populateInspector(ofxCvGui::InspectArguments& args) {
					auto inspector = args.inspector;

					inspector->addButton("Nudge all servos", [this]() {
						try {
							this->nudgeAllServos();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addButton("Zero all servos", [this]() {
						try {
							this->zeroAllServos();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

				//----------
				nlohmann::json Dispatcher::request(const ofHttpRequest & request) {
					ofURLFileLoader urlLoader;
					auto response = urlLoader.handleRequest(request);

					if (response.status != 200) {
						throw(ofxRulr::Exception(response.error));
					}

					auto responseJson = nlohmann::json::parse((string)response.data);
					if (!responseJson.contains("success")) {
						throw(ofxRulr::Exception("Malformed response from server : " + (string)response.data));
					}

					auto success = responseJson["success"];
					if ((bool)success) {
						if (responseJson.contains("data")) {
							return responseJson["data"];
						}
						else {
							return nlohmann::json();
						}
					}
					else {
						if (responseJson.contains("exception")) {
							throw(ofxRulr::Exception(responseJson["exception"].dump(4)));
						}
						else {
							throw(ofxRulr::Exception("Unknown error in server request"));
						}
					}
				}

				//----------
				nlohmann::json Dispatcher::requestGET(const string& path) {
					ofHttpRequest request(this->parameters.address.get() + path, "");
					request.method = ofHttpRequest::Method::GET;
					request.contentType = "application/json";

					return this->request(request);
				}

				//----------
				nlohmann::json Dispatcher::requestPOST(const string& path, const nlohmann::json& requestJson) {
					ofHttpRequest request(this->parameters.address.get() + path, "");
					request.method = ofHttpRequest::Method::POST;
					request.body = requestJson.dump(4);
					request.contentType = "application/json";

					return this->request(request);
				}

				//----------
				void Dispatcher::nudgeAllServos() {
					this->requestGET("/Test/NudgeAllServos");
				}

				//----------
				void Dispatcher::zeroAllServos() {
					this->requestGET("/Test/ZeroAllServos");
				}

				//----------
				void Dispatcher::multiMoveRequest(const MultiMoveRequest& multiMoveRequest) {
					nlohmann::json requestJson;
					
					requestJson["movements"] = nlohmann::json::array();
					for(const auto & movement : multiMoveRequest.movements) {
						auto movementJson = nlohmann::json::object();
						movementJson["servoID"] = movement.servoID;
						movementJson["position"] = movement.position;
						requestJson["movements"].push_back(movementJson);
					}

					requestJson["waitUntilComplete"] = multiMoveRequest.waitUntilComplete;
					requestJson["epsilon"] = multiMoveRequest.epsilon;
					requestJson["timeout"] = multiMoveRequest.timeout;

					this->requestPOST("/Servo/MultiMove", requestJson);
				}
			}
		}
	}
}
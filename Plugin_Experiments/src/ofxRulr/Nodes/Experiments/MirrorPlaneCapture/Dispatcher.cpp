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

					inspector->addTitle("Do for all servos", ofxCvGui::Widgets::Title::H2);

					inspector->addButton("Nudge", [this]() {
						try {
							this->nudge();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addButton("Zero", [this]() {
						try {
							this->zero();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addButton("Torque on", [this]() {
						try {
							this->setTorqueEnabled(true);
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Torque off", [this]() {
						try {
							this->setTorqueEnabled(false);
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
				void Dispatcher::nudge() {
					this->requestGET("/DoForAll/Nudge");
				}

				//----------
				void Dispatcher::zero() {
					this->requestGET("/DoForAll/Zero");
				}

				//----------
				void Dispatcher::setTorqueEnabled(bool torqueEnabled) {
					// get the list of servos first
					auto servoIDs = this->getServoIDs();

					MultiSetRequest request;
					request.registerName = "Torque Enable";
					for (const auto servoID : servoIDs) {
						request.servoValues[servoID] = torqueEnabled ? 1 : 0;
					}
					this->multiSetRequest(request);
				}

				//----------
				vector<Dispatcher::ServoID> Dispatcher::getServoIDs() {
					auto response = this->requestGET("/System/GetServoIDs");
					vector<ServoID> servoIDs;
					response.get_to(servoIDs);
					return servoIDs;
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

				//----------
				vector<Dispatcher::RegisterValue>
					Dispatcher::multiGetRequest(const MultiGetRequest& multiGetRequest) {
					nlohmann::json requestJson;

					requestJson["servoIDs"] = multiGetRequest.servoIDs;
					requestJson["registerType"] = multiGetRequest.registerName;

					auto result = this->requestPOST("/Servo/MultiGet", requestJson);
					auto values = result.get<vector<Dispatcher::RegisterValue>>();
					return values;
				}

				//----------
				void
					Dispatcher::multiSetRequest(const MultiSetRequest& multiSetRequest) {
					nlohmann::json requestJson;

					requestJson["registerType"] = multiSetRequest.registerName;

					for (const auto& servoValue : multiSetRequest.servoValues) {
						requestJson["servoValues"][ofToString(servoValue.first)] = servoValue.second;
					}
					auto result = this->requestPOST("/Servo/MultiSet", requestJson);
				}
			}
		}
	}
}
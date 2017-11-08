#include "pch_RulrNodes.h"
#include "HTTPServerControl.h"

#include "ofxRulr/Graph/World.h"
#include "ofxWebWidgets.h"


namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			//----------
			HTTPServerControl::HTTPServerControl() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			HTTPServerControl::~HTTPServerControl() {

			}

			//----------
			string HTTPServerControl::getTypeName() const {
				return "Application::HTTPServerControl";
			}

			//----------
			void HTTPServerControl::init() {
				RULR_NODE_UPDATE_LISTENER;
			}

			//----------
			void HTTPServerControl::update() {
				auto & server = ofxWebWidgets::Server::X();
				if (!server.isRunning() && this->parameters.run) {
					auto serverParameters = ofxWebWidgets::Server::Parameters();
					server.start();
				}
			}

			/*
			//----------
			void getRequest(ofxHTTPServerResponse & httpRequest) {
				auto urlVector = ofSplitString(httpRequest.url, "/");
				auto path = Path(urlVector.begin(), urlVector.end());
				path.pop_front(); // clear the empty
				path.pop_front(); // clear the prefix

				Request request{
					RequestType::GET,
					path,
					json(),
					json()
				};

				try {
					this->handlers(path, request);
					json response = {
						{ "success", true },
						{ "data", request.response }
					};
					httpRequest.response = response.dump();
				}
				catch (const ofxRulr::TracebackException & e) {
					json response = {
						{ "success", false },
						{ "error", e.what() },
						{ "traceback", e.getTraceback() }
					};
					httpRequest.response = response.dump();
				}
				catch (const std::exception & e) {
					json response = {
						{"success", false},
						{"error", e.what()}
					};
					httpRequest.response = response.dump();
				}
				catch (...) {
					json response = {
						{ "success", false },
					};
					httpRequest.response = response.dump();
				}
			}

			//----------
			void listNodes(Data::Application::HTTP::Request & request) {
				auto patch = ofxRulr::Graph::World::X().getPatch();

				auto & nodeHosts = patch->getNodeHosts();

				json jsonNodes;
				for (auto & it : nodeHosts) {
					auto nodeHost = it.second;
					if (nodeHost) {
						json jsonNode;
						jsonNode["index"] = it.first;
						auto node = nodeHost->getNodeInstance();
						if (node) {
							jsonNode["defined"] = true;
							jsonNode["name"] = node->getName();
							jsonNode["typeName"] = node->getTypeName();

							jsonNodes.push_back(jsonNode);
						}
					}
				}
				request.response = json{
					{"nodeCount", nodeHosts.size()},
					{"nodes", jsonNodes}
				};
			}

			*/
		}
	}
}
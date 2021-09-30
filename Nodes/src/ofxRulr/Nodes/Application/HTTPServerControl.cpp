#include "pch_RulrNodes.h"
#ifndef DISABLE_OFXWEBWIDGETS
#include "HTTPServerControl.h"

#include "ofxRulr/Graph/World.h"

#include "ofxWebWidgets.h"
using namespace ofxWebWidgets;


namespace ofxRulr {
	namespace Nodes {
		namespace Application {
#pragma mark HTTPServerControl::RequestHandler
			//----------
			HTTPServerControl::RequestHandler & HTTPServerControl::RequestHandler::X() {
				auto instance = make_unique<RequestHandler>();
				return *instance;
			}

			//----------
			void HTTPServerControl::RequestHandler::handleRequest(const Request & request
				, shared_ptr<Response> & response) {
				auto urlVector = ofSplitString(request.url, "/");
				auto path = Path(urlVector.begin(), urlVector.end());
				path.pop_front(); // clear the empty
				path.pop_front(); // clear the prefix

				json responseBody;

				try {
					json data;

					const auto pathString = request.getPathString();
					if (pathString == "/listNodes") {
						data = this->listNodes();
					}
					else {
						//quit without constructing a response
						//this is the correct pattern when 'we don't handle this'
						return;
					}

					responseBody = {
						{ "success", true },
						{ "data", data }
					};
				}
				catch (const ofxRulr::TracebackException & e) {
					responseBody = {
						{ "success", false },
						{ "error", e.what() },
						{ "traceback", e.getTraceback() }
					};
				}
				catch (const std::exception & e) {
					responseBody = {
						{ "success", false },
						{ "error", e.what() }
					};
				}
				catch (...) {
					responseBody = {
						{ "success", false },
					};
				}

				response = make_shared<ResponseJson>(responseBody);
			}

			//----------
			HTTPServerControl::RequestHandler::RequestHandler() {

			}

			//----------
			json HTTPServerControl::RequestHandler::listNodes() {
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
				return json{
					{ "nodeCount", nodeHosts.size() },
					{ "nodes", jsonNodes }
				};
			}

#pragma mark HTTPServerControl
			//----------
			HTTPServerControl::HTTPServerControl() {
				RULR_NODE_INIT_LISTENER;

				static bool requestHandlerIsSetup = false;
				if (!requestHandlerIsSetup) {
					//setup handlers to the world
					auto & server = ofxWebWidgets::Server::X();
					server.addRequestHandler(&RequestHandler::X());
					requestHandlerIsSetup = true;
				}
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
					server.start(serverParameters);
				}
				else if (server.isRunning() && !this->parameters.run) {
					server.stop();
				}
			}
		}
	}
}

#endif

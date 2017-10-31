#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxWebWidgets.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			class HTTPServerControl : public Nodes::Base {
			public:
				class RequestHandler : public ofxWebWidgets::RequestHandler {
				public:
					static RequestHandler & X();
					void handleRequest(const ofxWebWidgets::Request & request, shared_ptr<ofxWebWidgets::Response> & response) override;
					RequestHandler();
				protected:
					json listNodes();

				};

				HTTPServerControl();
				virtual ~HTTPServerControl();
				string getTypeName() const override;
				void init();
				void update();
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> run { "Run", false};
					PARAM_DECLARE("HTTPServerControl", run)
				} parameters;
			};
		}
	}
}
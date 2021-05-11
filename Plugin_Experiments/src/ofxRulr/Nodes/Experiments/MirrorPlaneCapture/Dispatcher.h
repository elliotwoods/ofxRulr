#pragma once

#include "pch_Plugin_Experiments.h"
#include "ofxRulr/Solvers/HeliostatActionModel.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class Dispatcher : public Nodes::Base {
				public:
					struct MultiMoveRequest {
						struct Movement {
							int servoID;
							int position;
						};

						vector<Movement> movements;
						bool waitUntilComplete = false;
						int epsilon = 1;
						float timeout = 5.0f;
					};

					Dispatcher();
					string getTypeName() const override;

					void init();
					void populateInspector(ofxCvGui::InspectArguments&);

					nlohmann::json request(const ofHttpRequest&);
					nlohmann::json requestGET(const string& path);
					nlohmann::json requestPOST(const string& path, const nlohmann::json& requestJson);

					void nudgeAllServos();
					void zeroAllServos();

					void multiMoveRequest(const MultiMoveRequest&);
				protected:
					struct : ofParameterGroup {
						ofParameter<string> address{ "http://localhost:8000" };
					} parameters;
				};
			}
		}
	}
}
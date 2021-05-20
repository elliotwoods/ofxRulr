#pragma once

#include "pch_Plugin_Experiments.h"
#include "ofxRulr/Solvers/HeliostatActionModel.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class Dispatcher : public Nodes::Base {
				public:
					typedef int ServoID;
					typedef int RegisterValue;

					struct MultiMoveRequest {
						struct Movement {
							ServoID servoID;
							RegisterValue position;
						};

						vector<Movement> movements;
						bool waitUntilComplete = false;
						int epsilon = 1;
						float timeout = 5.0f;
					};

					struct MultiGetRequest {
						vector<ServoID> servoIDs;
						std::string registerName;
					};

					struct MultiSetRequest {
						map<ServoID, RegisterValue> servoValues;
						std::string registerName;
					};

					Dispatcher();
					string getTypeName() const override;

					void init();
					void populateInspector(ofxCvGui::InspectArguments&);

					nlohmann::json request(const ofHttpRequest&);
					nlohmann::json requestGET(const string& path);
					nlohmann::json requestPOST(const string& path, const nlohmann::json& requestJson);

					void nudge();
					void zero();
					void setTorqueEnabled(bool);

					vector<ServoID> getServoIDs();

					void multiMoveRequest(const MultiMoveRequest&);
					vector<RegisterValue> multiGetRequest(const MultiGetRequest&);
					void multiSetRequest(const MultiSetRequest&);
				protected:
					struct : ofParameterGroup {
						ofParameter<string> address{ "http://localhost:8000" };
					} parameters;
				};
			}
		}
	}
}
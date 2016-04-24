#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Utils/ControlSocket.h"
#include "ofxRulr/Utils/MeshProvider.h"

#include "ofxMultiTrack.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class Subscriber : public Item::RigidBody {
			public:
				Subscriber();
				string getTypeName() const override;
				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;

				void drawObject();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				shared_ptr<ofxMultiTrack::Subscriber> getSubscriber() const;
				const ofFloatPixels & getDepthToWorldLUT() const;
				const ofTexture & getPreviewTexture() const;

			protected:
				shared_ptr<ofxCvGui::Panels::Texture> previewPanel;
				shared_ptr<ofxCvGui::Panels::Widgets> uiPanel;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<string> publisherAddress{ "Publisher address", "127.0.0.1" };
						ofParameter<int> publisherPort{ "Publisher port",  ofxMultiTrack::Ports::NodeToServerDataRangeBegin };
						ofParameter<int> controlPort{ "Control port",  ofxMultiTrack::Ports::NodeControl };
						ofParameter<bool> connect{ "Connect", false };

						PARAM_DECLARE("Connection", publisherAddress, controlPort, publisherPort, connect);
					} connection;

					struct : ofParameterGroup {
						ofParameter<string> depthToWorldTableFile{ "Depth to World table file", "" };
						PARAM_DECLARE("Calibration", depthToWorldTableFile);
					} calibration;

					struct : ofParameterGroup {
						ofParameter<bool> bodies{ "Bodies", false };
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<int> downsampleExp{ "Downsample (exp)", 0, 0, 3 };
							PARAM_DECLARE("Mesh", enabled, downsampleExp);
						} mesh;
						PARAM_DECLARE("Draw", bodies, mesh );
					} draw;

					PARAM_DECLARE("Receiver", connection, calibration, draw);
				} parameters;

				shared_ptr<ofxMultiTrack::Subscriber> subscriber;
				unique_ptr<Utils::ControlSocket> controlSocket;

				ofFloatPixels depthToWorldLUT;
				ofTexture depthToWorldTexture;
				ofTexture previewTexture;

				ofShader worldShader;
				Utils::MeshProvider meshProvider;

				void depthToWorldTableFileCallback(string &);
				void loadDepthToWorldTableFile();
			};
		}
	}
}
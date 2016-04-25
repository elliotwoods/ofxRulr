#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Utils/MeshProvider.h"

#include "ofxMultiTrack.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			class Subscriber : public Item::RigidBody {
			public:
				Subscriber();
				virtual ~Subscriber();
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

				const ofTexture & getDepthTexture() const;
				const ofTexture & getIRTexture() const;

				const ofFloatColor & getSubscriberColor() const;

				void drawPointCloudCpu();

				struct PointCloudStyle {
					bool applyIndexColor;
					bool applyIR;
				};
				void drawPointCloudGpu(PointCloudStyle);
			protected:
				shared_ptr<ofxCvGui::Panels::Texture> previewPanel;
				shared_ptr<ofxCvGui::Panels::Widgets> uiPanel;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<string> publisherAddress{ "Publisher address", "127.0.0.1" };
						ofParameter<int> publisherPort{ "Publisher port",  ofxMultiTrack::Ports::NodeToServerDataRangeBegin };
						ofParameter<bool> connect{ "Connect", false };

						PARAM_DECLARE("Connection", publisherAddress, publisherPort, connect);
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
							ofParameter<float> maxDisparity{ "Maximum disparity [m]", 0.1, 0.0, 1.0f };
							
							struct : ofParameterGroup {
								ofParameter<bool> applyIndexColor { "Apply index color", true };
								ofParameter<bool> applyIR { "Apply IR", true };
								PARAM_DECLARE("Style", applyIndexColor, applyIR);
							} style;
						
							PARAM_DECLARE("Point cloud", enabled, downsampleExp, maxDisparity, style);
						} gpuPointCloud;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };							
							ofParameter<bool> applyIRTexture{ "Apply IR Texture", false };
							ofParameter<float> colorAmplitude{ "Color amplitude", 256.0f, 0.0f, 1024.0f };
							PARAM_DECLARE("CPU point cloud", enabled, applyIRTexture, colorAmplitude);
						} cpuPointCloud;

						PARAM_DECLARE("Draw", bodies, gpuPointCloud, cpuPointCloud);
					} draw;

					PARAM_DECLARE("Receiver", connection, calibration, draw);
				} parameters;

				shared_ptr<ofxMultiTrack::Subscriber> subscriber;

				ofFloatPixels depthToWorldLUT;
				ofTexture depthToWorldTexture;

				ofTexture depthTexture;
				ofTexture irTexture;

				ofShader & getWorldShader();
				Utils::MeshProvider meshProvider;

				ofFloatColor debugColor;

				void depthToWorldTableFileCallback(string &);
				void loadDepthToWorldTableFile();
			};
		}
	}
}
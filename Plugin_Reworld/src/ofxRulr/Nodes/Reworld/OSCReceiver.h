#pragma once

#include "ofxRulr.h"
#include "ofxOsc.h"
#include "ofxRulr/Solvers/Reworld/Navigate/PointToPoint.h"

#include "Router.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class OSCReceiver : public Nodes::Base {
			public:
				OSCReceiver();
				string getTypeName() const override;

				void init();
				void update();
				void drawWorldStage();
				void updatePreviewData();
				void clearPreviewData();

				void populateInspector(ofxCvGui::InspectArguments&);
			protected:
				struct : ofParameterGroup {
					ofParameter<bool> enabled{ "Enabled", false };
					ofParameter<int> port{ "Port", 5000 };
					ofParameter<float> maxRxTimePerFrame{ "Max Rx time per frame [s]", 1.0 }; // useful to avoid flooding
					ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::Reworld::Navigate::PointToPoint::defaultSolverSettings() };
					
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", true };
						ofParameter<bool> colorsEnabled{ "Colors enabled", true };
						ofParameter<float> radius{ "Radius", 0.05f, 0.0f, 1.0f };
						PARAM_DECLARE("Preview", enabled, colorsEnabled, radius);
					} preview;

					ofParameter<bool> buildPreview{ "Build preview", true };
					PARAM_DECLARE("Osc Receiver", enabled, port, maxRxTimePerFrame, solverSettings, preview);
				} parameters;

				unique_ptr<ofxOscReceiver> oscReceiver;

				struct {
					bool isFrameNew = false;
					bool notifyFrameNew = false;
					size_t rxCount = 0;
				} oscFrameNew;

				struct PreviewPoint {
					glm::vec3 position;
					glm::vec3 modulePosition;
					ofColor color; // module color
				};

				map<Router::Address, PreviewPoint> previewData;
				
				struct {
					glm::vec3 min;
					glm::vec3 max;
				} bounds;

				bool previewDataDirty = true;
				ofVboMesh preview;

			};
		}
	}
}
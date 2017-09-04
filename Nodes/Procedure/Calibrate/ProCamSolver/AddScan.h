#pragma once
#include "Node.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					class AddScan : public Node {
					public:
						struct DataPoint {
							ofVec2f camera;
							ofVec2f projector;
							uint8_t median;
						};

						AddScan();
						string getTypeName() const override;
						void init();
						void drawWorld();

						void serialize(Json::Value &);
						void deserialize(const Json::Value &);
						void populateInspector(ofxCvGui::InspectArguments &);

						vector<DataPoint> getFitPoints() const;
					protected:
						shared_ptr<Nodes::Base> getNode() override;
						void buildPreviewRays();

						struct : ofParameterGroup {
							ofParameter<float> includeForFitRatio{ "Include for fit ratio", 0.01f, 0.00001f, 1.0f };
							PARAM_DECLARE("AddScan", includeForFitRatio);
						} parameters;

						ofMesh previewRays;
					};
				}
			}
		}
	}
}
#pragma once

#include "ThreadedProcessNode.h"
#include "FindMarkerCentroids.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class AddMarkerFromStereo : public ofxRulr::Nodes::Base {
			public:
				AddMarkerFromStereo();
				string getTypeName() const override;

				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments &);

				glm::vec3 getMarkerPositionEstimation() const;
				void drawWorldStage();
			protected:
				void addMarker();
				ofxCvGui::PanelPtr panel;

				ofImage previewA;
				ofImage previewB;

				ofThreadChannel<shared_ptr<FindMarkerCentroidsFrame>> incomingFramesA;
				ofThreadChannel<shared_ptr<FindMarkerCentroidsFrame>> incomingFramesB;
				shared_ptr<FindMarkerCentroidsFrame> frameA;
				shared_ptr<FindMarkerCentroidsFrame> frameB;

				unique_ptr<glm::vec2> centroidA;
				unique_ptr<glm::vec2> centroidB;
				unique_ptr<glm::vec3> estimatedPosition;

				struct : ofParameterGroup {
					ofParameter<float> previewRadius{ "Preview radius", 0.02, 0, 10 };
					PARAM_DECLARE("AddMarkerFrameStereo", previewRadius);
				} parameters;
			};
		}
	}
}
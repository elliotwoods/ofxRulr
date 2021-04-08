#pragma once

#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class Planes : public Nodes::Base {
			public:
				struct Plane : public Utils::AbstractCaptureSet::BaseCapture {
					Plane::Plane();
					string getDisplayString() const override;
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);

					ofxRay::Plane getPlane();

					struct : ofParameterGroup {
						ofParameter<glm::vec3> center{ "Center", glm::vec3(0, 0, 0) };
						ofParameter<glm::vec3> normal{ "Normal", glm::vec3(0, 1, 0) };
						ofParameter<glm::vec3> up{ "Up", glm::vec3(0, 0, 1) };
						ofParameter<glm::vec2> scale{ "Scale", glm::vec2(1.0, 1.0) };
						ofParameter<bool> mask{ "Mask", false }; // Disable points from becoming part of dataset
						PARAM_DECLARE("Plane", center, normal, up, scale, mask);
					} parameters;
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
				};

				Planes();
				string getTypeName() const override;
				void init();

				ofxCvGui::PanelPtr getPanel() override;

				void drawWorldStage();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				vector<shared_ptr<Plane>> getPlanes() const;
			protected:
				Utils::CaptureSet<Plane> planes;
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
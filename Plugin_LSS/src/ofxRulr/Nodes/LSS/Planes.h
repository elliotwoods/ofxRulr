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
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					ofxRay::Plane getPlane();

					struct : ofParameterGroup {
						ofParameter<ofVec3f> center{ "Center", ofVec3f(0, 0, 0) };
						ofParameter<ofVec3f> normal{ "Normal", ofVec3f(0, 1, 0) };
						ofParameter<ofVec3f> up{ "Up", ofVec3f(0, 0, 1) };
						ofParameter<ofVec2f> scale{ "Scale", ofVec2f(1.0, 1.0) };
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
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				vector<shared_ptr<Plane>> getPlanes() const;
			protected:
				Utils::CaptureSet<Plane> planes;
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
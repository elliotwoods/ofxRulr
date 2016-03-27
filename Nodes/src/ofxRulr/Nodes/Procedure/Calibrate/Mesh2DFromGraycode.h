#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class Mesh2DFromGraycode : public Nodes::Base {
				public:
					Mesh2DFromGraycode();
					string getTypeName() const override;

					void init();
					ofxCvGui::PanelPtr getPanel() override;

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void triangulate();
				protected:
					ofxCvGui::PanelPtr view;
				};
			}
		}
	}
}
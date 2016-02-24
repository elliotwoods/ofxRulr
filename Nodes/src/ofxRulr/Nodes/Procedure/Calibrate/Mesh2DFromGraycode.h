#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class Mesh2DFromGraycode : public Nodes::Gives<ofMesh> {
				public:
					Mesh2DFromGraycode();
					string getTypeName() const override;

					void init();
					ofxCvGui::PanelPtr getView() override;

					void get(ofMesh &) const override;

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					void triangulate();
				protected:
					ofMesh mesh;
					ofxCvGui::PanelPtr view;
				};
			}
		}
	}
}
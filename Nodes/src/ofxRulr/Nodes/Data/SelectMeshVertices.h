#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			class SelectMeshVertices : public Base {
			public:
				class VertexPicker : public ofxCvGui::Element {
				public:
					VertexPicker(SelectMeshVertices * node);
					struct {
						ofVec3f screenPosition;
						ofVec3f worldPosition;
						bool found = false;
					} selectedVertex;
				};

				class Vertex : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Vertex();
					void drawWorld();
					string getDisplayString() const override;
					ofParameter<ofVec3f> position;
				};

				SelectMeshVertices();
				string getTypeName() const override;
				void init();
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void drawWorld();
				virtual ofxCvGui::PanelPtr getPanel() override;
			protected:
				shared_ptr<VertexPicker> selectNewVertex;
				Utils::CaptureSet<Vertex> pickedVertices;
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
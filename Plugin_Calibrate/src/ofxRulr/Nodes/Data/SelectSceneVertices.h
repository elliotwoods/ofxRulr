#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/IReferenceVertices.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			class SelectSceneVertices : public Procedure::Calibrate::IReferenceVertices {
			public:
				class VertexPicker : public ofxCvGui::Element {
				public:
					VertexPicker(SelectSceneVertices * node);
					struct {
						glm::vec3 screenPosition;
						glm::vec3 worldPosition;
						bool found = false;
					} selectedVertex;

					ofxLiquidEvent<glm::vec3> onVertexFound;
				};

				SelectSceneVertices();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();
			protected:
				void setNewVertexPosition(const glm::vec3 &);
				shared_ptr<Vertex> newVertex;
				shared_ptr<VertexPicker> selectNewVertex;
				shared_ptr<ofxCvGui::Widgets::Button> addVertexButton;
			};
		}
	}
}
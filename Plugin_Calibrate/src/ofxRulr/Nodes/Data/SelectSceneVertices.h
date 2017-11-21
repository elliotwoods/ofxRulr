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
						ofVec3f screenPosition;
						ofVec3f worldPosition;
						bool found = false;
					} selectedVertex;

					ofxLiquidEvent<ofVec3f> onVertexFound;
				};

				SelectSceneVertices();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();
			protected:
				void setNewVertexPosition(const ofVec3f &);
				shared_ptr<Vertex> newVertex;
				shared_ptr<VertexPicker> selectNewVertex;
				shared_ptr<ofxCvGui::Widgets::Button> addVertexButton;
			};
		}
	}
}
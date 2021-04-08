#pragma once

#include "ofxRulr.h"

#include "ofxCvGui/Panels/Image.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Nodes/Procedure/Base.h"

#include "Constants_Plugin_Calibration.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class PLUGIN_CALIBRATE_EXPORTS IReferenceVertices : public Procedure::Base {
				public:
					class Vertex : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Vertex();
						void drawWorld(bool selected = false);
						string getDisplayString() const override;
						ofParameter<glm::vec3> worldPosition{ "World", glm::vec3(0, 0, 0) };
						ofParameter<glm::vec2> viewPosition{ "View", glm::vec2(0, 0) };
						ofParameter<string> name{ "Name", "" };
						void setOwner(shared_ptr<IReferenceVertices>);
					protected:
						void drawObjectLines();
						weak_ptr<IReferenceVertices> owner;
					};

					IReferenceVertices();
					string getTypeName() const override;
					void init();
					void drawWorldStage();
					ofxCvGui::PanelPtr getPanel() override;

					vector<shared_ptr<Vertex>> getSelectedVertices() const;
					shared_ptr<Vertex> getNextVertex(shared_ptr<Vertex>, int direction = 1) const;
					ofxLiquidEvent<void> onChangeVertex;
				protected:
					void addVertex(shared_ptr<Vertex>);
					Utils::CaptureSet<Vertex> vertices;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;
				};
			}
		}
	}
}
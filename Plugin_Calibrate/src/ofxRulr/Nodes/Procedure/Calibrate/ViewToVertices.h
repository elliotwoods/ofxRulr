#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/IReferenceVertices.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class ViewToVertices : public Base {
				public:
					MAKE_ENUM(Action
						, (CalibrateCamera, SolvePnp)
						, ("CalibrateCamera", "SolvePnP"));

					class ViewArea : public ofBaseDraws {
					public:
						ViewArea(ViewToVertices &);
						void draw(float x, float y) const override;
						void draw(float x, float y, float w, float h) const override;
						float getHeight() const override;
						float getWidth() const override;
					protected:
						ViewToVertices & parent;
					};
					ViewToVertices();
					string getTypeName() const override;
					void init();
					ofxCvGui::PanelPtr getPanel() override;
					void update();
					void drawWorldStage();
					void remoteControl(RemoteControllerArgs& args);

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					void populateInspector(ofxCvGui::InspectArguments &);

					void calibrate(); // will throw on fail
				protected:
					void calibrateCalibrateCamera(const vector<shared_ptr<IReferenceVertices::Vertex>>& vertices);
					void calibrateSolvePnP(const vector<shared_ptr<IReferenceVertices::Vertex>>& vertices);
					void drawOnProjector();
					void centerOnVertex();

					ViewArea viewArea;
					shared_ptr<ofxCvGui::Panels::Draws> view;
						
					ofParameter<filesystem::path> projectorReferenceImageFilename{ "Projector reference image filename", "" };

					struct : ofParameterGroup {
						ofParameter<bool> dragVerticesEnabled{ "Drag vertices enabled", true };
						ofParameter<bool> calibrateOnVertexChange{ "Calibrate on vertex change", true };
						ofParameter<bool> useExistingParametersAsInitial{ "Use existing data as initial", false };
						ofParameter<Action> action{ "Action", Action::CalibrateCamera };
						PARAM_DECLARE("ViewToVertices"
							, dragVerticesEnabled
							, calibrateOnVertexChange
							, useExistingParametersAsInitial
							, action);
					} parameters;
					
					ofImage projectorReferenceImage;
					bool success;
					float reprojectionError;
					weak_ptr<IReferenceVertices::Vertex> selection;
				};
			}
		}
	}
}

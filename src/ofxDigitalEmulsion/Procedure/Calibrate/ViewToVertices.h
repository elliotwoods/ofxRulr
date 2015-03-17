#pragma once

#include "../Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class ViewToVertices : public Base {
			public:
				class ViewArea : public ofBaseDraws {
				public:
					ViewArea(ViewToVertices &);
					void draw(float x, float y) override;
					void draw(float x, float y, float w, float h) override;
					float getHeight() override;
					float getWidth() override;
				protected:
					ViewToVertices & parent;
				};
				ViewToVertices();
				string getTypeName() const override;
				void init();
				ofxCvGui::PanelPtr getView() override;
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::ElementGroupPtr);

				void calibrate(); // will throw on fail
			protected:
				void drawOnProjector();

				ViewArea viewArea;
				ofxCvGui::PanelPtr view;
			
				ofParameter<string> projectorReferenceImageFilename;
				ofImage projectorReferenceImage;
				ofParameter<bool> dragVerticesEnabled;
				ofParameter<bool> calibrateOnVertexChange;
				ofParameter<bool> useExistingParametersAsInitial;
				bool success;
				float reprojectionError;
			};
		}
	}
}

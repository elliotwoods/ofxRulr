#pragma once

#include "Base.h"

#include <ofxCvGui/Panels/World.h>

#include "ofxAssimpModelLoader.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Model : public Base {
		public:
			Model();
			string getTypeName() const override;
			void init() override;
			ofxCvGui::PanelPtr getView() override;

			void update() override;
			void drawWorld() override;

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);

			void updatePreviewMesh();

			shared_ptr<ofxCvGui::Panels::World> view;

			ofParameter<string> filename;

			ofParameter<bool> drawVertices, drawWireframe, drawFaces;

			ofParameter<bool> flipX, flipY, flipZ;
			ofParameter<float> inputUnitScale;

			ofxAssimpModelLoader modelLoader;
			ofLight light;
		};
	}
}
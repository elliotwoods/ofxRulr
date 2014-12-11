#pragma once

#include "Base.h"

#include "ofxAssimpModelLoader.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Model : public Base {
		public:
			Model();
			string getTypeName() const override;
			void init() override;
			ofxCvGui::PanelPtr getView();
			void drawWorld() override;

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;

			void updatePreviewMesh();

			ofParameter<string> filename;

			ofxAssimpModelLoader modelLoader;
		};
	}
}
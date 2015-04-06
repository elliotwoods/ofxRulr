#pragma once

#include "Base.h"

#include "ofxCvGui/Panels/World.h"

#include "../../../addons/ofxRay/src/ofxRay.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		class Triangulate : public Base {
		public:
			Triangulate();
			void init();
			string getTypeName() const override;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			void triangulate();
		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);
			void drawWorld();
			
			ofMesh mesh;
			
			ofParameter<float> maxLength;
			ofParameter<bool> giveColor;
			ofParameter<bool> giveTexCoords;
			ofParameter<float> drawPointSize;
		};
	}
}
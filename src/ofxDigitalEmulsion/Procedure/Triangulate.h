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

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			void triangulate();
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;
			void drawWorld();
			
			ofMesh mesh;
			
			ofxRay::Ray cameraRay, projectorRay, intersectRay;

			ofParameter<float> maxLength;
			ofParameter<bool> giveColor;
			ofParameter<bool> giveTexCoords;
			ofParameter<float> drawPointSize;
			
			ofParameter<bool> drawDebugRays;
		};
	}
}
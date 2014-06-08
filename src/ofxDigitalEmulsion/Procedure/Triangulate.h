#pragma once

#include "Base.h"

#include "ofxCvGui/Panels/World.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		class Triangulate : public Base {
		public:
			Triangulate();
			string getTypeName() const override;
			Graph::PinSet getInputPins() const override;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			void triangulate();
		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;

			Graph::PinSet inputPins;

			ofMesh mesh;

			ofParameter<float> maxLength;
			ofParameter<bool> giveColor;
			ofParameter<bool> giveTexCoords;
			ofParameter<float> drawPointSize;
		};
	}
}
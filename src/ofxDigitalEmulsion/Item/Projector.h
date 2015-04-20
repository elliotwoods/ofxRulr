#pragma once

#include "View.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Projector : public View {
		public:
			Projector();
			void init();
			string getTypeName() const;

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);

			void updateProjectorFromParameters();
			void updateParametersFromProjector();
			void projectorParameterCallback(float &);
			
			ofxRay::Projector projector;
		};
	}
}
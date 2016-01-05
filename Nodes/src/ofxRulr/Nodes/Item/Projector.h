#pragma once

#include "View.h"

#include "ofxCvMin.h"
#include "ofxRay.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class Projector : public View {
			public:
				Projector();
				void init();
				string getTypeName() const;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);

				void updateProjectorFromParameters();
				void updateParametersFromProjector();
				void projectorParameterCallback(float &);

				ofxRay::Projector projector;
			};
		}
	}
}
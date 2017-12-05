#pragma once

#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class Scan : public Nodes::Base {
			public:
				Scan();
				string getTypeName() const override;
				void init();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void scan();
			protected:
				ofMatrix4x4 lastCameraTransform;

				struct : ofParameterGroup {
					ofParameter<bool> useExistingData{ "Use existing data", false };
					PARAM_DECLARE("Scan", useExistingData);
				} parameters;
			};
		}
	}
}
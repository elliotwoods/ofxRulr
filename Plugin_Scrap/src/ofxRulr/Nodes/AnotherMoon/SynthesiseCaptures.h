#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class SynthesiseCaptures : public Nodes::Base {
			public:
				SynthesiseCaptures();
				string getTypeName() const override;

				void init();
				void populateInspector(ofxCvGui::InspectArguments&);

				void synthesise();

				struct : ofParameterGroup {
					ofParameter<int> resolution{ "Resolution", 4 };
					ofParameter<float> spread{ "Spread", 1 };
					ofParameter<bool> disableExistingCaptures{ "Disable existing captures", true };
					PARAM_DECLARE("SynthesiseCaptures", resolution, spread, disableExistingCaptures);
				} parameters;
			};
		}
	}
}
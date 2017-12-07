#pragma once

#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class FitLines : public Nodes::Base {
			public:
				FitLines();
				string getTypeName() const override;
				void init();

				void populateInspector(ofxCvGui::InspectArguments &);
				void fit();
				void pick();
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> distance{ "Distance normalized", 0.01, 0.001, 0.5 };
						PARAM_DECLARE("Pick", distance);
					} pick;
					PARAM_DECLARE("FitLines", pick);
				} parameters;
			};
		}
	}
}
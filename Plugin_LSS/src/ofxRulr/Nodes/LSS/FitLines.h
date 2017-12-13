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
				void fullAuto();
				void saveResults();
			protected:
				struct : ofParameterGroup {
// 					struct : ofParameterGroup {
// 						ofParameter<bool> removeOutliners{ "Remove outliers", true };
// 						ofParameter<float> threshold{ "Threshold [%]", 0.1, 0, 1};
// 						PARAM_DECLARE("Outliers", remove, threshold);
// 					} outliers;

					struct : ofParameterGroup {
						ofParameter<float> maxResidualForFit{ "Max residual for fit", 0.01 };
						ofParameter<float> distance{ "Distance normalized", 0.01, 0.001, 0.5 };
						PARAM_DECLARE("Pick", maxResidualForFit, distance);
					} pick;
					PARAM_DECLARE("FitLines", pick);
				} parameters;
			};
		}
	}
}
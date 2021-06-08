#pragma once

#include "pch_Plugin_Experiments.h"
#include "ofxRulr/Solvers/HeliostatActionModel.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class TrackCursor : public Nodes::Base {
				public:
					TrackCursor();
					string getTypeName() const override;

					void init();
					void update();
					void populateInspector(ofxCvGui::InspectArguments&);

					void track();

				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> printReport{ "Print report", false };
							ofParameter<float> parameterTolerance{ "Parameter tolerance", 1e-2 };
							PARAM_DECLARE("Navigator", printReport, parameterTolerance);
						} navigator;

						ofParameter<WhenDrawOnWorldStage> perform{ "Perform", WhenDrawOnWorldStage::Never };
						PARAM_DECLARE("Track Cursor", navigator, perform);
					} parameters;
				};
			}
		}
	}
}
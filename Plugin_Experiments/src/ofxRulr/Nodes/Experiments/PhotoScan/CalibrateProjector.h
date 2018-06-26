#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace PhotoScan {
				class CalibrateProjector : public Base {
				public:
					CalibrateProjector();
					string getTypeName() const override;
					void init();
					void calibrate();
					ofxCvGui::PanelPtr getPanel() override;
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();

				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<float> threshold{ "Threshold", 5, 0, 255 };
								ofParameter<int> dilations{ "Dilations", 3, 0, 10 };
								PARAM_DECLARE("Mask", threshold);
							} mask;
							PARAM_DECLARE("Discontinuity filtering", mask);
						} disocontinuityFiltering;

						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<float> lensOffset{ "Lens offset", 0, -1.0, 1.0 };
								ofParameter<float> throwRatio{ "Throw ratio", 1, 0, 5 };
								PARAM_DECLARE("Initial", lensOffset, throwRatio);
							} initial;

							PARAM_DECLARE("Calibrate", initial);
						} calibrate;

						PARAM_DECLARE("CalibrateProjector", disocontinuityFiltering, calibrate);
					} parameters;

					ofxCvGui::PanelPtr panel;

					ofParameter<float> reprojectionError{ "Reprojection error [px]", 0, 0, 100.0 };

					struct {
						ofImage mask;
					} preview;
				};
			}
		}
	}
}
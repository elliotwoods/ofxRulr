#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Models/Reworld/Module.h"
#include "ofxRulr/Data/Reworld/Module.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class SimulateLightBeams : public Base
			{
			public:
				SimulateLightBeams();
				string getTypeName() const override;

				void init();
				void update();
				void drawWorldStage();
				void populateInspector(ofxCvGui::InspectArguments);

				void simulate();
				void clearResult();
				void rebuildPreview();
			protected:
				struct : ofParameterGroup {
					ofParameter<WhenActive> autoPerform{ "Auto perform", WhenActive::Never };
					
					struct : ofParameterGroup {
						ofParameter<float> extendExitRay{ "Extend exit ray", 10, 1, 100 };
						PARAM_DECLARE("Preview", extendExitRay);
					} preview;

					PARAM_DECLARE("SimulateLightBeans", autoPerform, preview);
				} parameters;

				struct Result {
					ofxCeres::Models::Ray<float> incomingRay;
					Models::Reworld::Module<float>::RefractionResult refractionResult;
					weak_ptr<Data::Reworld::Module> module;
				};

				vector<Result> refractionResults;
				ofVboMesh refractionResultsPreview;
			};
		}
	}
}
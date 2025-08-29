#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Solvers/Reworld/Navigate/PointToPoint.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class NavigatePointToPoint : public Base
			{
			public:
				NavigatePointToPoint();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments);

				void perform();
			protected:
				struct : ofParameterGroup {
					ofParameter<WhenActive> performAutomatically{ "Perform automatically", WhenActive::Never };
					ofParameter<bool> performOnTargetChange{ "Perform on target change", false };
					ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::Reworld::Navigate::PointToPoint::defaultSolverSettings() };

					PARAM_DECLARE("LookAtPointToPoint", performAutomatically, performOnTargetChange, solverSettings);
				} parameters;

				bool needsPerform = false;
			};
		}
	}
}
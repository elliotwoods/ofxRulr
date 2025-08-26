#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class LookAtPointToPoint : public Base
			{
			public:
				LookAtPointToPoint();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments);

				void perform();
			protected:
				struct : ofParameterGroup {
					ofParameter<WhenActive> performAutomatically{ "Perform automatically", WhenActive::Never };
					PARAM_DECLARE("LookAtPointToPoint", performAutomatically);
				} parameters;
			};
		}
	}
}
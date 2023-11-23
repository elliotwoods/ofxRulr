#pragma once
#pragma once

#include "pch_Plugin_Experiments.h"
#include "Heliostats.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class RemoteControl : public Nodes::Base {
				public:
					RemoteControl();
					string getTypeName() const override;

					void init();
					void update();
					void drawWorldStage();

					void populateInspector(ofxCvGui::InspectArguments&);

					void remoteControl(const RemoteControllerArgs&);

					shared_ptr<Heliostats2::Heliostat> getSingle();
					void moveSingle(const glm::vec2& movement);
					void moveSelection(const glm::vec2& movement);
					void homeSingle();
					void homeSelection();

					void flipSingle();
					void flipSelection();

					void move(shared_ptr<Heliostats2::Heliostat>, const glm::vec2& movement);
					void home(shared_ptr<Heliostats2::Heliostat>);
				protected:
					struct : ofParameterGroup {
						ofParameter<string> singleName{ "Single name", "70" };
						ofParameter<float> speedKeyboard{ "Speed keyboard", 1, 0, 10 };
						ofParameter<float> speedGameController { "Speed game controller", 1, 0, 10};
						PARAM_DECLARE("Remote control", singleName, speedKeyboard, speedGameController);
					} parameters;
				};
			}
		}
	}
}
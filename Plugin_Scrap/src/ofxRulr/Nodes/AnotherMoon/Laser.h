#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxOsc.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Lasers;

			class Laser
				: public Utils::AbstractCaptureSet::BaseCapture
				, public ofxCvGui::IInspectable
				, public enable_shared_from_this<Laser>
			{
			public:
				MAKE_ENUM(State
					, (Shutdown, Standby, Run)
					, ("Shutdown", "Standby", "Run"));

				MAKE_ENUM(Source
					, (Circle, USB, Memory)
					, ("Circle", "USB", "Memory"));

				Laser();
				~Laser();
				void setParent(Lasers*);

				string getDisplayString() const override;

				void update();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);
				void drawWorldStage();

				shared_ptr<Item::RigidBody> getRigidBody();

				void shutown();
				void standby();
				void run();

				void setBrightness(float);
				void setSize(float);
				void setSource(const Source&);

				void drawCircle(glm::vec2 center, float radius);

				string getHostname() const;
				void sendMessage(const ofxOscMessage&);

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<int> address{ "Address", 0 };
						ofParameter<glm::vec2> fov{ "FOV", {30, 30} };
						ofParameter<glm::vec2> centerOffset{ "Center offset", {0, 0} };
						PARAM_DECLARE("Settings", address, fov, centerOffset);
					} settings;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<WhenActive> centerLine{ "Center line", WhenActive::Selected };
							PARAM_DECLARE("Draw", centerLine);
						} draw;
						PARAM_DECLARE("Debug", draw);
					} debug;

					PARAM_DECLARE("Laser", settings, debug);
				} parameters;
			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;

				Lasers* parent;
				unique_ptr<ofxOscSender> oscSender;
				shared_ptr<Item::RigidBody> rigidBody = make_shared<Item::RigidBody>();
			};
		}
	}
}
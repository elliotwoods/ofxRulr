#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxOrbbec.h"

#include "ofxCvGui/Panels/Groups/Strip.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace Orbbec {
					class ColorRegistration;
				}
			}
		}
		namespace Item {
			namespace Orbbec {
				class Device : public Nodes::Item::RigidBody {
				public:
					Device();
					string getTypeName() const override;

					void init();
					void update();

					void drawObject();

					ofxCvGui::PanelPtr getPanel() override;

					shared_ptr<ofxOrbbec::Device> getDevice();
				
				protected:
					friend Procedure::Calibrate::Orbbec::ColorRegistration;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> color{ "Color", true };
							ofParameter<bool> depth{ "Depth", true };
							ofParameter<bool> infrared{ "Infrared", false };
							ofParameter<bool> points{ "Points", true };
							ofParameter<bool> skeleton{ "Skeleton", true };
							PARAM_DECLARE("Enabled streams", color, depth, infrared, points, skeleton);
						} enabledStreams;
						PARAM_DECLARE("Orbbec", enabledStreams);
					} parameters;

					bool streamsNeedRebuilding = true;
					void rebuildStreams();
					void streamEnableCallback(bool &);

					shared_ptr<ofxCvGui::Panels::Groups::Strip> panelStrip;
					shared_ptr<ofxOrbbec::Device> device;
				};
			}
		}
	}
}
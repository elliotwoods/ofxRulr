#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"

#include "ofxKinectForWindows2.h"

#include "ofxCvGui/Panels/Groups/Strip.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class KinectV2 : public ofxRulr::Nodes::Item::RigidBody {
			public:
				MAKE_ENUM(PlayState,
					(Play, Pause),
					("Play", "Pause"));

				MAKE_ENUM(DrawType,
					(Frustums, Bodies, All),
					("Frustums", "Bodies", "All"));

				KinectV2();
				void init();
				string getTypeName() const override;
				void update();
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				void drawObject();
				shared_ptr<ofxKinectForWindows2::Device> getDevice();

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void rebuildView();

				shared_ptr<ofxKinectForWindows2::Device> device;
				shared_ptr<ofxCvGui::Panels::Groups::Strip> view;

				struct : ofParameterGroup {
					ofParameter<PlayState> playState{ "Play state", PlayState::Play };
					ofParameter<DrawType> viewType{ "View type", DrawType::All };

					struct : ofParameterGroup {
						ofParameter<bool> rgb{ "RGB", true };
						ofParameter<bool> depth{ "Depth", true };
						ofParameter<bool> ir{ "IR", false };
						ofParameter<bool> body{ "Body", false };
						PARAM_DECLARE("Enabled views", rgb, depth, ir, body);
					} enabledViews;

					PARAM_DECLARE("KinectV2", playState, viewType, enabledViews);
				} parameters;
			};
		}
	}
}
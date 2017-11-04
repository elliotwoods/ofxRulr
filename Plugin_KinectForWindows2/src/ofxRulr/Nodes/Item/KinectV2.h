#pragma once

#include "ofxRulr/Nodes/Item/RigidBody.h"

#include "ofxKinectForWindows2.h"

#include "ofxCvGui/Panels/Groups/Strip.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			class KinectV2 : public ofxRulr::Nodes::Item::RigidBody {
			public:
				KinectV2();
				void init();
				string getTypeName() const override;
				void update();
				ofxCvGui::PanelPtr getPanel() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void drawObject();
				shared_ptr<ofxKinectForWindows2::Device> getDevice();

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				void rebuildView();

				shared_ptr<ofxKinectForWindows2::Device> device;
				shared_ptr<ofxCvGui::Panels::Groups::Strip> view;

				ofParameter<int> playState;
				ofParameter<int> viewType;

				struct : ofParameterGroup {
					ofParameter<bool> rgb{ "RGB", true };
					ofParameter<bool> depth{ "Depth", true };
					ofParameter<bool> ir{ "IR", false };
					ofParameter<bool> body{ "Body", false };
					PARAM_DECLARE("Enabled views", rgb, depth, ir, body);
				} enabledViews;
			};
		}
	}
}
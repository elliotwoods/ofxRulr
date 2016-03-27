#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Draws.h"

#include "of3dPrimitives.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			class ARCube : public Nodes::Base {
			public:
				ARCube();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void drawWorld() override;

				ofxCvGui::PanelPtr getPanel() override;

				bool getRunFinderEnabled() const;
			protected:
				shared_ptr<ofxCvGui::Panels::Draws> view;
				ofImage undistorted;

				ofMatrix4x4 boardTransform;
				bool foundBoard;
				ofFbo fbo;

				// 0 = when selected
				// 1 = always
				ofParameter<int> activewhen;
				// 0 - axes
				// 1 - board
				// 2 - cube
				ofParameter<int> drawStyle;

				ofBoxPrimitive cube;
			};
		}
	}
}
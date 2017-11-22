#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

#include "ofxCvGui/Panels/Draws.h"
#include "ofxCvGui/Utils/Enum.h"

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

				void drawWorldStage();

				ofxCvGui::PanelPtr getPanel() override;

				bool getRunFinderEnabled() const;
			protected:
				shared_ptr<ofxCvGui::Panels::Draws> view;
				ofImage undistorted;

				ofMatrix4x4 boardTransform;
				bool foundBoard;
				ofFbo fbo;

				MAKE_ENUM(ActiveWhen
					, (Selected, Always)
					, ("Selected", "Always"));
				MAKE_ENUM(DrawStyle
					, (Axes, Board, Cube)
					, ("Axes", "Board", "Cube"));
				MAKE_ENUM(FillMode
					, (Fill, Wireframe)
					, ("Fill", "Wireframe"));

				struct : ofParameterGroup {
					ofParameter<ActiveWhen> activewhen{ "Active when", ActiveWhen::Selected };
					ofParameter<DrawStyle> drawStyle{ "Draw style", DrawStyle::Cube };
					ofParameter<FillMode> fillMode{ "Fill mode", FillMode::Fill };
					ofParameter<float> alpha{ "Alpha", 1.0f, 0.0f, 1.0f };
					ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
					PARAM_DECLARE("ARCube", activewhen, drawStyle, fillMode, alpha, findBoardMode);
				} parameters;

				ofBoxPrimitive cube;
			};
		}
	}
}
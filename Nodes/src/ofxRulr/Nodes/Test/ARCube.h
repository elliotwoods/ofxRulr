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
				void updateTracking();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

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
					ofParameter<DrawStyle> drawStyle{ "Draw style", DrawStyle::Board };
					ofParameter<FillMode> fillMode{ "Fill mode", FillMode::Fill };
					ofParameter<float> alpha{ "Alpha", 0.5f, 0.0f, 1.0f };
					ofParameter<FindBoardMode> findBoardMode{ "Find board mode", FindBoardMode::Optimized };
					
					struct : ofParameterGroup {
						ofParameter<bool> drawImagePoints{ "Draw image points", false };
						ofParameter<bool> drawObjectPoints{ "Draw object points", false };
						ofParameter<bool> drawProjectedPoints{ "Draw projected points", false };
						ofParameter<bool> drawUndistortedPoints{ "Draw undistorted", false };
						PARAM_DECLARE("Debug", drawImagePoints, drawObjectPoints, drawProjectedPoints, drawUndistortedPoints);
					} debug;

					PARAM_DECLARE("ARCube", activewhen, drawStyle, fillMode, alpha, findBoardMode, debug);
				} parameters;

				ofBoxPrimitive cube;
				float reprojectionError = 0.0f;
				vector<cv::Point3f> objectPoints;
				vector<cv::Point2f> imagePoints;
				vector<cv::Point2f> projectedPoints;
				vector<cv::Point2f> projectedUndistortedPoints;
			};
		}
	}
}
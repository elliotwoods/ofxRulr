#pragma once

#include "../Nodes/Base.h"

#include "ofxCvGui/Panels/World.h"

#include "ofLight.h"

namespace ofxRulr {
	namespace Graph {
		class RULR_EXPORTS WorldStage : public Nodes::Base {
		public:
			WorldStage();
			string getTypeName() const override;
			void init();

			void setWorld(const Utils::Set<Nodes::Base> &);

			void update();
			ofxCvGui::PanelPtr getPanel() override;
#ifdef OFXCVGUI_USE_OFXGRABCAM
			ofVec3f getCursorWorld(bool forceUpdate = false) const;
			ofxGrabCam & getCamera();
#endif
		protected:
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::InspectArguments &);
#ifdef OFXCVGUI_USE_OFXGRABCAM
			void callbackShowCursor(bool &);
#endif
			void callbackGridDark(bool &);

			void drawGrid();

			shared_ptr<ofxCvGui::Panels::World> view;

			struct : ofParameterGroup {
				ofParameter<bool> showCursor{ "Show cursor",false };
				struct : ofParameterGroup {
					ofParameter<bool> enabled{ "Enabled", true };
					ofParameter<ofVec3f> roomMinimum{ "Room minimum", ofVec3f(-5, -4, 0) };
					ofParameter<ofVec3f> roomMaximum{ "Room maximum", ofVec3f(+5, 0, 6) };
					ofParameter<bool> dark{ "Dark", true };
					PARAM_DECLARE("Grid", enabled, roomMinimum, roomMaximum, dark);
				} grid;
				
				PARAM_DECLARE("WorldStage", showCursor, grid);
			} parameters;

			const Utils::Set<Nodes::Base> * world;

			ofCamera * camera = nullptr;
			ofTexture * grid;
			ofLight light;
		};
	}
}
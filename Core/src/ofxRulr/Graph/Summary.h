#pragma once

#include "../Nodes/Base.h"

#include "ofxCvGui/Panels/World.h"

#include "ofLight.h"

namespace ofxRulr {
	namespace Graph {
		class Summary : public Nodes::Base {
		public:
			Summary();
			string getTypeName() const override;
			void init();

			void setWorld(const Utils::Set<Nodes::Base> &);

			void update();
			ofxCvGui::PanelPtr getPanel() override;
		protected:
			void serialize(Json::Value &);
			void deserialize(const Json::Value &);
			void populateInspector(ofxCvGui::InspectArguments &);
#ifdef OFXCVGUI_USE_OFXGRABCAM
			void callbackShowCursor(bool &);
#endif

			void drawGrid();

			shared_ptr<ofxCvGui::Panels::World> view;

			ofParameter<bool> showCursor;
			ofParameter<bool> showGrid;
			ofParameter<ofVec3f> roomMinimum;
			ofParameter<ofVec3f> roomMaximum;

			const Utils::Set<Nodes::Base> * world;

			ofImage * grid;
			ofLight light;
		};
	}
}
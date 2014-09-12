#pragma once

#include "../Utils/Set.h"
#include "../Graph/Node.h"

#include "ofxCvGui/Controller.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class World : public Utils::Set<Graph::Node> {
		public:
			void init(ofxCvGui::Controller &);
			void loadAll(bool printDebug = false);
			void saveAll() const;
			static ofxCvGui::Controller & getGuiController();
			ofxCvGui::PanelGroupPtr getGuiGrid();
		protected:
			static ofxCvGui::Controller * gui;
			ofxCvGui::PanelGroupPtr guiGrid;
		};
	}
}
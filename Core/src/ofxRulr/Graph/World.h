#pragma once

#include "../Utils/Set.h"
#include "../Nodes/Base.h"

#include "ofxCvGui/Controller.h"
#include "ofxCvGui/Panels/SharedView.h"
#include "ofxSingleton.h"

namespace ofxRulr {
	namespace Graph {
		class World : public Utils::Set<Nodes::Base>, public ofxSingleton::Singleton<World> {
		public:
			void init(ofxCvGui::Controller &);
			void loadAll(bool printDebug = false);
			void saveAll() const;
			static ofxCvGui::Controller & getGuiController();
			ofxCvGui::PanelGroupPtr getGuiGrid() const;
		protected:
			static ofxCvGui::Controller * gui; ///< Why is this static? Needs comment.  I presume it's so we can grid multiple worlds?
			ofxCvGui::PanelGroupPtr guiGrid;
		};
	}
}
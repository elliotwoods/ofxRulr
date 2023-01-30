#pragma once

#include "../Utils/Set.h"
#include "../Nodes/Base.h"
#include "Editor/Patch.h"

#include "WorldStage.h"

#include "ofxCvGui/Controller.h"
#include "ofxCvGui/Panels/SharedView.h"
#include "ofxSingleton.h"

namespace ofxRulr {
	namespace Graph {
		class OFXRULR_API_ENTRY World : public Utils::Set<Nodes::Base>, public ofxSingleton::Singleton<World> {
		public:
			World();
			virtual ~World();
			void init(ofxCvGui::Controller &, bool enableWorldStageView = true);
			void loadAll(bool printDebug = false);
			void saveAll() const;
			static ofxCvGui::Controller & getGuiController();
			ofxCvGui::PanelGroupPtr getGuiGrid() const;
			shared_ptr<Editor::Patch> getPatch() const;
			shared_ptr<WorldStage> getWorldStage() const;

			void drawWorld() const;
			void drawWorldAdvanced(DrawWorldAdvancedArgs&) const;

			ofParameter<bool> lockSelection{ "Lock selection", false };
		protected:
			static ofxCvGui::Controller * gui; ///< Why is this static? Needs comment.  I presume it's so we can grid multiple worlds?
			ofxCvGui::PanelGroupPtr guiGrid;
			chrono::system_clock::time_point lastSaveOrLoad = chrono::system_clock::now();

			shared_ptr<WorldStage> worldStage;
		};
	}
}
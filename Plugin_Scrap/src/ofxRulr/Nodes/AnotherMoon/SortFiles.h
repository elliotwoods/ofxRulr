#pragma once

#include "ofxRulr.h"

#include "Calibrate.h"


namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class SortFiles : public Nodes::Base {
				MAKE_ENUM(MoveOrCopy
					, (Move, Copy)
					, ("Move", "Copy"));
				MAKE_ENUM(OnError
					, (DisableBeam, End)
					, ("DisableBeam", "End"));
			public:
				SortFiles();
				string getTypeName() const override;

				void init();

				void sortFiles();
			protected:
				struct : ofParameterGroup {
					ofParameter<filesystem::path> targetDirectory{ "Target directory", filesystem::path() };
					ofParameter<MoveOrCopy> moveOrCopy{ "Move or copy", MoveOrCopy::Move };
					ofParameter<bool> selectionOnly{ "Selection only", false };
					ofParameter<bool> dryRun{ "Dry run", false };
					ofParameter<bool> verbose{ "Verbose", true };
					ofParameter<bool> openFileFirst{ "Open file first", true };
					ofParameter<bool> dontOverwrite{ "Don't overwrite", true };
					
					ofParameter<OnError> onError{ "On error", OnError::DisableBeam };

					PARAM_DECLARE("SortFiles"
						, targetDirectory
						, moveOrCopy
						, selectionOnly
						, dryRun
						, verbose
						, onError
						, openFileFirst
						, dontOverwrite);
				} parameters;
			};
		}
	}
}
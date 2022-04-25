#pragma once

#include "ofxRulr.h"

#include "Calibrate.h"


namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class SortFiles : public Nodes::Base {
			public:
				SortFiles();
				string getTypeName() const override;

				void init();

				void sortFiles();
			protected:
				struct : ofParameterGroup {
					ofParameter<filesystem::path> targetDirectory{ "Target directory", filesystem::path() };
					ofParameter<bool> moveFiles{ "Move files", false };
					ofParameter<bool> selectionOnly{ "Selection only", false };
					ofParameter<bool> dryRun{ "Dry run", false };
					ofParameter<bool> verbose{ "Verbose", true };
					ofParameter<bool> stopOnException{ "Stop on exception", true };
					ofParameter<bool> openFileFirst{ "Open file first", true };
					PARAM_DECLARE("SortFiles"
						, targetDirectory
						, moveFiles
						, selectionOnly
						, dryRun
						, verbose
						, stopOnException
						, openFileFirst);
				} parameters;
			};
		}
	}
}
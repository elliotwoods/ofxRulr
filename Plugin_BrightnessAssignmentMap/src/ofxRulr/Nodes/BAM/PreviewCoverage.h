#pragma once

#include "ofxRulr/Nodes/Render/Style.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			class PreviewCoverage : public Nodes::Base {
			public:
				PreviewCoverage();
				string getTypeName() const override;
				void init();
				void drawWorldStage();
			protected:
			};
		}
	}
}
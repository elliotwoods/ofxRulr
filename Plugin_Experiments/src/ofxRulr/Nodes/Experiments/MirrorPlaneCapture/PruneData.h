#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class PruneData : public Nodes::Base {
				public:
					PruneData();
					string getTypeName() const override;

					void init();
					ofxCvGui::PanelPtr getPanel() override;
				protected:
					ofxCvGui::PanelPtr panel;
				};
			}
		}
	}
}
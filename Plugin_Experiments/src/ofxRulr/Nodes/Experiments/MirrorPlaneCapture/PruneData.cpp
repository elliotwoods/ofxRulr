#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				//----------
				PruneData::PruneData()
				{
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string
					PruneData::getTypeName() const
				{
					return "Halo::PruneData";
				}

				//----------
				void
					PruneData::init()
				{
					this->panel = ofxCvGui::Panels::makeBlank();

				}

				//----------
				ofxCvGui::PanelPtr
					PruneData::getPanel()
				{
				}
			}
		}
	}
}

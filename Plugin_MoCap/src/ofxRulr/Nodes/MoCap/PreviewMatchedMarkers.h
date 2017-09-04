#pragma once

#include "ThreadedProcessNode.h"
#include "MatchMarkers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class PreviewMatchedMarkers : public ThreadedProcessNode<MatchMarkers
				, MatchMarkersFrame
				, void *> {
			public:
				PreviewMatchedMarkers();
				string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
			protected:
				void processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) override;
				ofxCvGui::PanelPtr panel;

				shared_ptr<MatchMarkersFrame> previewFrame;
				mutex previewFrameMutex;

				atomic<bool> previewDirty = true;
				ofImage preview;
			};
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ThreadedProcessNode.h"
#include "FindMarkerCentroids.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class PreviewCentroids : public ThreadedProcessNode<FindMarkerCentroids
			, FindMarkerCentroidsFrame
			, void*> {
			public:
				PreviewCentroids();
				virtual ~PreviewCentroids();
				string getTypeName() const override;
				void init();
				void update();

				virtual ofxCvGui::PanelPtr getPanel() override;
			protected:
				ofImage previewImage;
				
				shared_ptr<FindMarkerCentroidsFrame> previewFrame;
				mutex previewFrameMutex;
				atomic<bool> previewImageDirty = false;

				ofxCvGui::PanelPtr panel;

				void processFrame(shared_ptr<FindMarkerCentroidsFrame> incomingFrame) override;

				struct : ofParameterGroup {
					ofParameter<bool> drawBounds{ "Draw bounds", true };
					ofParameter<bool> drawText{ "Draw text", true };
					ofParameter<bool> drawCentroids{ "Draw centroids", true };
					PARAM_DECLARE("PreviewCentroids", drawBounds, drawText, drawCentroids);
				} parameters;
			};
		}
	}
}
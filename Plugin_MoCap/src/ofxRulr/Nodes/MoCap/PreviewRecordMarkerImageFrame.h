#pragma once

#include "ThreadedProcessNode.h"
#include "RecordMarkerImages.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class PreviewRecordMarkerImagesFrame : public ThreadedProcessNode<RecordMarkerImages
				, RecordMarkerImagesFrame
				, void *> {
			public:
				PreviewRecordMarkerImagesFrame();
				virtual string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
			protected:
				void processFrame(shared_ptr<RecordMarkerImagesFrame>) override;
				void drawOnImage(ofxCvGui::DrawImageArguments & args);
				shared_ptr<RecordMarkerImagesFrame> previewFrame;
				mutex previewFrameMutex;

				ofImage image;
				ofImage blurred;
				ofImage difference;
				ofImage binary;
				
				vector<ofMesh> contours;
				vector<ofPath> filteredContours;

				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
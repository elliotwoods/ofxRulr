#pragma once

#include "ThreadedProcessNode.h"
#include "MatchMarkers.h"
#include "Body.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			struct UpdateTrackingFrame {
				shared_ptr<MatchMarkersFrame> incomingFrame;

				cv::Mat modelViewRotationVector;
				cv::Mat modelViewTranslation;
				ofMatrix4x4 transform;

			};

			class UpdateTracking : public ThreadedProcessNode<MatchMarkers
				, MatchMarkersFrame
				, UpdateTrackingFrame> {
			public:
				UpdateTracking();
				string getTypeName() const override;
				void init();
				void update();
			protected:
				void processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) override;

				shared_ptr<Body> bodyNode;
				mutex bodyNodeMutex;
			};
		}
	}
}
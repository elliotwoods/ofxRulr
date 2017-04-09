#pragma once

#include "ThreadedProcessNode.h"
#include "MatchMarkers.h"
#include "Body.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			MAKE_ENUM(UpdateTarget
				, (Body, Camera)
				, ("Body", "Camera"));

			struct UpdateTrackingFrame {
				shared_ptr<MatchMarkersFrame> incomingFrame;

				UpdateTarget updateTarget;

				cv::Mat bodyModelViewRotationVector;
				cv::Mat bodyModelViewTranslation;

				cv::Mat modelRotationVector;
				cv::Mat modelTranslation;
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

				struct : ofParameterGroup {
					ofParameter<UpdateTarget> updateTarget{ "Update target", UpdateTarget::Camera };
					PARAM_DECLARE("UpdateTracking", updateTarget);
				} parameters;

				ofThreadChannel<shared_ptr<UpdateTrackingFrame>> trackingUpdateToMainThread;
			};
		}
	}
}
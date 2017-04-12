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

				vector<cv::Point2f> reprojectedAfterTracking;

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
				void populateInspector(ofxCvGui::InspectArguments &);
			protected:
				void processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) override;

				struct : ofParameterGroup {
					ofParameter<UpdateTarget> updateTarget{ "Update target", UpdateTarget::Camera };
					ofParameter<float> reprojectionThreshold{ "Reprojection threshold [px]", 5 };
					PARAM_DECLARE("UpdateTracking", updateTarget, reprojectionThreshold);
				} parameters;

				ofThreadChannel<shared_ptr<UpdateTrackingFrame>> trackingUpdateToMainThread;
				atomic<float> reprojectionError;
			};
		}
	}
}
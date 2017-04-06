#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/ThreadPool.h"

#include "StereoSolvePnP.h"

#include "MatchMarkers.h"
#include "UpdateTracking.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/StereoCalibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			class UpdateTrackingStereo : public Nodes::Base {
			public:
				UpdateTrackingStereo();
				string getTypeName() const override;
				void init();
				void update();
				void populateInspector(ofxCvGui::InspectArguments &);

				//happens in 'our thread'
				ofxLiquidEvent<shared_ptr<UpdateTrackingFrame>> onNewFrame;
			protected:
				atomic<int> processedFramesSinceLastAppFrame = 0;
				atomic<int> droppedFramesSinceLastAppFrame = 0;
				unique_ptr<Utils::ThreadPool> threadPool;
				float processedFramesPerSecond = 0.0f;
				float droppedFramesPerSecond = 0.0f;

				void processFrame(shared_ptr<MatchMarkersFrame> incomingFrame, bool cameraIndex);
				void processCameraSet(vector<shared_ptr<MatchMarkersFrame>>, shared_ptr<MatchMarkersFrame> incomingFrame);

				atomic<bool> cameraWhichSendsSecond = 1;

				unique_ptr<StereoSolvePnP> stereoSolvePnP;

				shared_ptr<Body> bodyNode;
				mutex bodyNodeMutex;

				shared_ptr<Procedure::Calibrate::StereoCalibrate> stereoCalibrateNode;
				mutex stereoCalibrateNodeMutex;

				vector<shared_ptr<MatchMarkersFrame>> markerTracking{ 2 };
				mutex markerTrackingMutex;

				ofThreadChannel<float> computeTimeChannel;
				float computeTime;
			};
		}
	}
}
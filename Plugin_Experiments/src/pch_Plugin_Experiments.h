#pragma once

#include "pch_RulrNodes.h"
#include "pch_RulrCore.h"

#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/BoardInWorld.h"

#include "ofxRulr/Nodes/Experiments/SolvePnP/TestObject.h"
#include "ofxRulr/Nodes/Experiments/SolvePnP/ProjectPoints.h"
#include "ofxRulr/Nodes/Experiments/SolvePnP/SolvePnP.h"

#include "ofxRulr/Nodes/Experiments/SolveMirror/SolveMirror.h"

#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/BoardInMirror.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/BoardInMirror2.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/BoardOnMirror.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/HaloBoard.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/Heliostats.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/Heliostats2.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/SolarAlignment.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/Dispatcher.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/NavigateBodyToBody.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/TrackCursor.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/RemoteControl.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/SunTracker.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/SunCalibrator.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/Halo.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/NavigateToHalo.h"
#include "ofxRulr/Nodes/Experiments/MirrorPlaneCapture/PruneData.h"

#include "ofxRulr/Nodes/Experiments/PhotoScan/BundlerCamera.h"
#include "ofxRulr/Nodes/Experiments/PhotoScan/CalibrateProjector.h"

#include "ofxRulr/Nodes/Experiments/ProCamSolve/SolveProjector.h"

#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"

#include "ofxRulr/Solvers/HeliostatActionModel.h"
#include "ofxRulr/Solvers/MirrorPlaneFromRays.h"
#include "ofxRulr/Solvers/RotationFrame.h"

#include "pch_Plugin_Calibrate.h"
#include "pch_Plugin_ArUco.h"

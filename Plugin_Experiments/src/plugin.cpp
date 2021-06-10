#include "pch_Plugin_Experiments.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
{
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::SolvePnP::TestObject);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::SolvePnP::ProjectPoints);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::SolvePnP::SolvePnP);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::SolveMirror::SolveMirror);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::BoardInMirror);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::BoardInMirror2);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::BoardOnMirror);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::HaloBoard);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::Heliostats);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::Heliostats2);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::SolarAlignment);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::NavigateBodyToBody);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::TrackCursor);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::RemoteControl);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::Halo);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::SunTracker);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::SunCalibrator);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::NavigateToHalo);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::MirrorPlaneCapture::Dispatcher);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::PhotoScan::BundlerCamera);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::PhotoScan::CalibrateProjector);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Experiments::ProCamSolve::SolveProjector);
}
OFXPLUGIN_PLUGIN_MODULES_END
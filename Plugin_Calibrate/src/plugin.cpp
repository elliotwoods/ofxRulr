#include "pch_Plugin_Calibrate.h"

OFXPLUGIN_PLUGIN_MODULES_BEGIN(ofxRulr::Nodes::Base)
{
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Data::SelectSceneVertices);

	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::CameraExtrinsicsFromBoardInWorld);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::CameraExtrinsicsFromBoardPlanes);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::CameraFromDepthCamera);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::CameraIntrinsics);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::HomographyFromGraycode);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::Mesh2DFromGraycode);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::MovingHeadToWorld);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ProjectorFromDepthCamera);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ProjectorFromGraycode);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ProjectorFromStereoAndHelperCamera);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ProjectorFromStereoCamera);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::StereoCalibrate);
	OFXPLUGIN_PLUGIN_REGISTER_MODULE(ofxRulr::Nodes::Procedure::Calibrate::ViewToVertices);
}
OFXPLUGIN_PLUGIN_MODULES_END

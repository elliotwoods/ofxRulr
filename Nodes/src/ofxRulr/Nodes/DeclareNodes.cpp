#include "pch_RulrNodes.h"
#include "DeclareNodes.h"

#include "ofxRulr/Nodes/Application/Assets.h"

#include "ofxRulr/Nodes/Data/Channels/Database.h"
#include "ofxRulr/Nodes/Data/Channels/Generator/Application.h"
#include "ofxRulr/Nodes/Data/Mesh.h"
#include "ofxRulr/Nodes/Data/Recorder.h"

#include "ofxRulr/Nodes/DMX/Sharpy.h"
#include "ofxRulr/Nodes/DMX/AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Nodes/Item/Camera.h"
//#include "Nodes/Item/Model.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/Item/View.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/CameraIntrinsics.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/HomographyFromGraycode.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/Mesh2DFromGraycode.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ViewToVertices.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/MovingHeadToWorld.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/ProjectorFromDepthCamera.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/CameraFromDepthCamera.h"
#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Procedure/Triangulate.h"

#include "ofxRulr/Nodes/Render/NodeThroughView.h"

#include "ofxRulr/Nodes/System/VideoOutput.h"

#include "ofxRulr/Nodes/Test/ARCube.h"
#include "ofxRulr/Nodes/Test/Focus.h"

#include "ofxRulr/Graph/FactoryRegister.h"

namespace ofxRulr {
	namespace Nodes {
		void loadCoreNodes() {
			RULR_DECLARE_NODE(Application::Assets);
			
			RULR_DECLARE_NODE(Data::Channels::Database);
			RULR_DECLARE_NODE(Data::Channels::Generator::Application);
			RULR_DECLARE_NODE(Data::Mesh);
			RULR_DECLARE_NODE(Data::Recorder);

			RULR_DECLARE_NODE(DMX::Sharpy);
			RULR_DECLARE_NODE(DMX::AimMovingHeadAt);

			RULR_DECLARE_NODE(Item::Board);
			RULR_DECLARE_NODE(Item::Camera);
			//RULR_DECLARE_NODE(Item::Model);
			RULR_DECLARE_NODE(Item::Projector);
			RULR_DECLARE_NODE(Item::RigidBody);
			RULR_DECLARE_NODE(Item::View);

			RULR_DECLARE_NODE(Procedure::Calibrate::CameraIntrinsics);
			RULR_DECLARE_NODE(Procedure::Calibrate::HomographyFromGraycode);
			RULR_DECLARE_NODE(Procedure::Calibrate::Mesh2DFromGraycode);
			RULR_DECLARE_NODE(Procedure::Calibrate::ViewToVertices);
			RULR_DECLARE_NODE(Procedure::Calibrate::MovingHeadToWorld);
			RULR_DECLARE_NODE(Procedure::Calibrate::ProjectorFromDepthCamera);
			RULR_DECLARE_NODE(Procedure::Calibrate::CameraFromDepthCamera);
			RULR_DECLARE_NODE(Procedure::Scan::Graycode);
			RULR_DECLARE_NODE(Procedure::Triangulate);

			RULR_DECLARE_NODE(Render::NodeThroughView);

			RULR_DECLARE_NODE(System::VideoOutput);
			
			RULR_DECLARE_NODE(Test::ARCube);
			RULR_DECLARE_NODE(Test::Focus);
		}

		void loadPluginNodes() {
			Graph::FactoryRegister::X().loadPlugins();
		}
	}
}
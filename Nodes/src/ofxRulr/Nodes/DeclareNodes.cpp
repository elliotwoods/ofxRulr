#include "pch_RulrNodes.h"
#include "DeclareNodes.h"

#include "ofxRulr/Nodes/Template.h"

#include "ofxRulr/Nodes/Application/Assets.h"
#include "ofxRulr/Nodes/Application/openFrameworks.h"
#include "ofxRulr/Nodes/Application/HTTPServerControl.h"
#include "ofxRulr/Nodes/Application/Debugger.h"

#include "ofxRulr/Nodes/Data/Channels/Database.h"
#include "ofxRulr/Nodes/Data/Channels/Generator/Application.h"
#include "ofxRulr/Nodes/Data/Mesh.h"
#include "ofxRulr/Nodes/Data/Recorder.h"

#include "ofxRulr/Nodes/DMX/Sharpy.h"
#include "ofxRulr/Nodes/DMX/AimMovingHeadAt.h"

#include "ofxRulr/Nodes/Item/Board.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Mesh.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Nodes/Item/View.h"
#include "ofxRulr/Nodes/Item/BoardInWorld.h"
#include "ofxRulr/Nodes/Item/RandomPatternBoard.h"
#include "ofxRulr/Nodes/Item/Grid.h"

#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Procedure/Triangulate.h"

#include "ofxRulr/Nodes/Render/NodeThroughView.h"
#include "ofxRulr/Nodes/Render/WorldThroughView.h"
#include "ofxRulr/Nodes/Render/Draw.h"
#include "ofxRulr/Nodes/Render/Lighting.h"

#include "ofxRulr/Nodes/System/VideoOutput.h"

#include "ofxRulr/Nodes/Test/ARCube.h"
#include "ofxRulr/Nodes/Test/Focus.h"

#include "ofxRulr/Nodes/Watchdog/Camera.h"
#include "ofxRulr/Nodes/Watchdog/Startup.h"

#include "ofxRulr/Graph/FactoryRegister.h"

namespace ofxRulr {
	namespace Nodes {
		void loadCoreNodes() {
			RULR_DECLARE_NODE(Template);

			RULR_DECLARE_NODE(Application::Assets);
			RULR_DECLARE_NODE(Application::openFrameworks);
#ifndef DISABLE_OFXWEBWIDGETS
			RULR_DECLARE_NODE(Application::HTTPServerControl);
#endif
			RULR_DECLARE_NODE(Application::Debugger);
			
			RULR_DECLARE_NODE(Data::Channels::Database);
			RULR_DECLARE_NODE(Data::Channels::Generator::Application);
			RULR_DECLARE_NODE(Data::Mesh);
			RULR_DECLARE_NODE(Data::Recorder);

			RULR_DECLARE_NODE(DMX::Sharpy);
			RULR_DECLARE_NODE(DMX::AimMovingHeadAt);

			RULR_DECLARE_NODE(Item::Board);
			RULR_DECLARE_NODE(Item::Camera);
			RULR_DECLARE_NODE(Item::Mesh);
			RULR_DECLARE_NODE(Item::Projector);
			RULR_DECLARE_NODE(Item::RigidBody);
			RULR_DECLARE_NODE(Item::View);
			RULR_DECLARE_NODE(Item::BoardInWorld);
			RULR_DECLARE_NODE(Item::RandomPatternBoard);
			RULR_DECLARE_NODE(Item::Grid);

			RULR_DECLARE_NODE(Procedure::Scan::Graycode);

			RULR_DECLARE_NODE(Procedure::Triangulate);

			RULR_DECLARE_NODE(Render::NodeThroughView);
			RULR_DECLARE_NODE(Render::WorldThroughView);
			RULR_DECLARE_NODE(Render::Draw);
			RULR_DECLARE_NODE(Render::Lighting);

			RULR_DECLARE_NODE(System::VideoOutput);
			
			RULR_DECLARE_NODE(Test::ARCube);
			RULR_DECLARE_NODE(Test::Focus);

			RULR_DECLARE_NODE(Watchdog::Camera);
			RULR_DECLARE_NODE(Watchdog::Startup);
		}

		void loadPluginNodes() {
			Graph::FactoryRegister::X().loadPlugins();
		}
	}
}
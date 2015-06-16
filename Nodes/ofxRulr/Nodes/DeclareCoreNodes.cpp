#include "DeclareCoreNodes.h"
#include "ofxRulr/Graph/Factory.h"

#include "Nodes/Device/VideoOutput.h"

#include "Nodes/Item/Board.h"
#include "Nodes/Item/Camera.h"
//#include "Nodes/Item/Model.h"
#include "Nodes/Item/Projector.h"
#include "Nodes/Item/RigidBody.h"
#include "Nodes/Item/View.h"

#include "Nodes/Procedure/Calibrate/CameraIntrinsics.h"
#include "Nodes/Procedure/Calibrate/HomographyFromGraycode.h"
#include "Nodes/Procedure/Calibrate/ViewToVertices.h"
#include "Nodes/Procedure/Scan/Graycode.h"
#include "Nodes/Procedure/Triangulate.h"

namespace ofxRulr {
	namespace Nodes {
		void declareCoreNodes() {
			RULR_DECLARE_NODE(Device::VideoOutput);

			RULR_DECLARE_NODE(Item::Board);
			RULR_DECLARE_NODE(Item::Camera);
			//RULR_DECLARE_NODE(Item::Model);
			RULR_DECLARE_NODE(Item::Projector);
			RULR_DECLARE_NODE(Item::RigidBody);
			RULR_DECLARE_NODE(Item::View);

			RULR_DECLARE_NODE(Procedure::Calibrate::CameraIntrinsics);
			RULR_DECLARE_NODE(Procedure::Calibrate::HomographyFromGraycode);
			RULR_DECLARE_NODE(Procedure::Calibrate::ViewToVertices);
			RULR_DECLARE_NODE(Procedure::Scan::Graycode);
			RULR_DECLARE_NODE(Procedure::Triangulate);
		}
	}
}
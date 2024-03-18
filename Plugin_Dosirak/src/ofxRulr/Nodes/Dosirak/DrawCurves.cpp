#include "pch_Plugin_Dosirak.h"
#include "DrawCurves.h"
#include "Curves.h"
#include "ofxRulr/Nodes/AnotherMoon/Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			//----------
			DrawCurves::DrawCurves()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				DrawCurves::getTypeName() const
			{
				return "Dosirak::DrawCurves";
			}

			//----------
			void
				DrawCurves::init()
			{
				this->manageParameters(this->parameters);

				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<AnotherMoon::Lasers>();
				auto curvesInput = this->addInput<Curves>();

				curvesInput->onNewConnection += [this](shared_ptr<Curves> curvesNode) {
					curvesNode->onNewCurves.addListener([this]() {
						if (this->parameters.onNewFrame.get()) {
							this->needsDrawCurves = true;
						}
						}, this);
				};

				curvesInput->onDeleteConnection += [this](shared_ptr<Curves> curvesNode) {
					if (curvesNode) {
						curvesNode->onNewCurves.removeListeners(this);
					}
				};
			}

			//----------
			void
				DrawCurves::update()
			{
				if (this->needsDrawCurves) {
					try {
						this->drawCurves();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void
				DrawCurves::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Draw curves", [this]() {
					try {
						this->drawCurves();
					}
					RULR_CATCH_ALL_TO_ERROR;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			void
				DrawCurves::drawCurves()
			{
				auto curvesNode = this->getInput<Curves>();
				auto lasersNode = this->getInput<AnotherMoon::Lasers>();

				if (curvesNode && lasersNode) {

					// gather world points
					vector<glm::vec3> worldPoints;
					{
						auto curves = curvesNode->getCurvesTransformed();
						for (const auto& curve : curves) {
							worldPoints.insert(worldPoints.begin(), curve.second.points.begin(), curve.second.points.end());
						}
					}

					// send world points to all laser projectors
					auto lasers = lasersNode->getLasersSelected();
					for (auto laser : lasers) {
						laser->drawWorldPoints(worldPoints);
					}
				}

				this->needsDrawCurves = false;
			}
		}
	}
}
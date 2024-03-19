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

				const auto useFastMethod = this->parameters.useFastMethod.get();

				if (curvesNode && lasersNode) {

					// gather world points
					auto worldPoints = curvesNode->getWorldPoints(curvesNode->getCurvesRaw());

					// send world points to all laser projectors
					auto lasers = lasersNode->getLasersSelected();
					for (auto laser : lasers) {
						auto picture = useFastMethod
							? laser->renderWorldPointsFast(worldPoints)
							: laser->renderWorldPoints(worldPoints);
						laser->drawPicture(picture);
					}
				}

				this->needsDrawCurves = false;
			}
		}
	}
}
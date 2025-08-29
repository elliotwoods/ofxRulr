#include "pch_Plugin_Reworld.h"
#include "SimulateLightBeams.h"
#include "Installation.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			SimulateLightBeams::SimulateLightBeams()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				SimulateLightBeams::getTypeName() const
			{
				return "Reworld::SimulateBeams";
			}

			//----------
			void
				SimulateLightBeams::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Installation>();
				this->addInput<Item::RigidBody>("Light source");

				this->manageParameters(this->parameters);
			}

			//----------
			void
				SimulateLightBeams::update()
			{
				if (ofxRulr::isActive(this, this->parameters.autoPerform)) {
					try {
						this->simulate();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void
				SimulateLightBeams::drawWorldStage()
			{
				if (this->refractionResultsPreview.getNumVertices() > 0) {
					this->refractionResultsPreview.draw();
				}
			}

			//----------
			void
				SimulateLightBeams::populateInspector(ofxCvGui::InspectArguments args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Simulate", [this]() {
					try {
						this->simulate();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, ' ')->setHeight(100.0f);
				inspector->addButton("Clear result", [this]() {
					this->clearResult();
					});
				inspector->addButton("Rebuild preview", [this]() {
					this->rebuildPreview();
					});
			}

			//----------
			void
				SimulateLightBeams::simulate()
			{
				this->throwIfMissingAnyConnection();
				auto installationNode = this->getInput<Installation>();
				auto lightSourceNode = this->getInput<Item::RigidBody>("Light source");

				auto modules = installationNode->getSelectedModules();

				auto lightSourcePosition = lightSourceNode->getPosition();

				this->clearResult();

				this->refractionResults.reserve(modules.size());

				for (auto module : modules) {
					auto model = module->getModel();
					auto modulePosition = ofxCeres::VectorMath::applyTransform(model.getTransform(), glm::vec3( 0, 0, 0 ));
					
					ofxCeres::Models::Ray<float> incomingRay;
					{
						incomingRay.s = lightSourcePosition;
						incomingRay.t = glm::normalize(modulePosition - lightSourcePosition);
					}

					auto simulationResult = model.refract(
						incomingRay
						, {
							module->parameters.axisAngles.A.get()
						, module->parameters.axisAngles.B.get()
						});
					
					Result result;
					{
						result.incomingRay = incomingRay;
						result.refractionResult = simulationResult;
						result.module = module;
					}
					this->refractionResults.push_back(result);
				}

				this->rebuildPreview();
			}

			//----------
			void
				SimulateLightBeams::clearResult()
			{
				this->refractionResults.clear();
				this->rebuildPreview();
			}

			//----------
			void
				SimulateLightBeams::rebuildPreview()
			{
				this->refractionResultsPreview.clear();
				this->refractionResultsPreview.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				const auto& extendExitRay = this->parameters.preview.extendExitRay.get();

				for (const auto& result : this->refractionResults) {
					auto module = result.module.lock();
					auto color = (ofFloatColor) ((bool)(module) ? module->color.get() : ofColor(255, 255, 255));

					refractionResultsPreview.addColor(color);
					refractionResultsPreview.addVertex(result.incomingRay.getStart());
					refractionResultsPreview.addColor(color);
					refractionResultsPreview.addVertex(result.incomingRay.getEnd());


					refractionResultsPreview.addColor(color);
					refractionResultsPreview.addVertex(result.refractionResult.intermediateRay.getStart());
					refractionResultsPreview.addColor(color);
					refractionResultsPreview.addVertex(result.refractionResult.intermediateRay.getEnd());

					refractionResultsPreview.addColor(color);
					refractionResultsPreview.addVertex(result.refractionResult.outputRay.getStart());
					refractionResultsPreview.addColor(color);
					refractionResultsPreview.addVertex(result.refractionResult.outputRay.s + result.refractionResult.outputRay.t * extendExitRay);
				}
			}
		}
	}
}
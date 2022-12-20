#include "pch_Plugin_Caustics.h"
#include "BlockSolver.h"
#include "SimpleSurface.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Caustics {
			//----------
			BlockSolver::BlockSolver()
			{
				RULR_NODE_INIT_LISTENER;
				this->setIconGlyph(u8"\uf279");
			}

			//----------
			string
				BlockSolver::getTypeName() const
			{
				return "Caustics::BlockSolver";
			}

			//----------
			void BlockSolver::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_UPDATE_LISTENER;

				this->manageParameters(this->parameters);

				this->addInput<SimpleSurface>();

			}

			//----------
			void BlockSolver::update()
			{
				if (this->parameters.sectionSolve.continuously) {
					try {
						for (int i = 0; i < this->parameters.sectionSolve.iterations.get(); i++) {
							this->solve();
						}
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void
				BlockSolver::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Solve", [this]() {
					try {
						this->solve();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN);

				inspector->addButton("Reset", [this]() {
					try {
						this->reset();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
			}

			//----------
			void
				BlockSolver::serialize(nlohmann::json& json)
			{
			}

			//----------
			void
				BlockSolver::deserialize(const nlohmann::json& json)
			{
			}
			
			//----------
			void
				BlockSolver::solve()
			{
				this->throwIfMissingAnyConnection();
				auto simpleSurface = this->getInput<SimpleSurface>();

				auto i_start = this->parameters.sectionSolve.i.get();
				auto j_start = this->parameters.sectionSolve.j.get();
				auto width = this->parameters.sectionSolve.width.get();
				auto height = this->parameters.sectionSolve.height.get();

				// Pull out the surface
				Models::Surface subSurface;
				auto & surface = simpleSurface->getSurface();
				{
					subSurface.distortedGrid = surface.distortedGrid.getSubSection(i_start
						, j_start
						, width
						, height);
				}

				// Perform solve and push the data back
				{
					auto result = Solvers::NormalsSurface::solveByIndividual(subSurface
						, this->parameters.sectionSolve.solverSettings.getSolverSettings());

					surface.distortedGrid.setSubSection(i_start
						, j_start
						, result.solution.surface.distortedGrid);

					simpleSurface->updatePreview();
				}

				// Update section selection (so we iterate through)
				if(this->parameters.sectionSolve.move) {
					bool reachedEnd = false;

					i_start += width;
					if (i_start >= surface.distortedGrid.cols()) {
						i_start = 0;
						j_start += height;
						if (j_start >= surface.distortedGrid.rows()) {
							j_start = 0;
							reachedEnd = true;
						}
					}

					this->parameters.sectionSolve.i.set(i_start);
					this->parameters.sectionSolve.j.set(j_start);

					if (reachedEnd && this->parameters.sectionSolve.once) {
						this->parameters.sectionSolve.continuously.set(false);
					}
				}
			}

			//----------
			void
				BlockSolver::reset()
			{
				this->parameters.sectionSolve.i.set(0);
				this->parameters.sectionSolve.j.set(0);
			}
		}
	}
}
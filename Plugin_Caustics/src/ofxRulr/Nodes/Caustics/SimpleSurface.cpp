#include "pch_Plugin_Caustics.h"
#include "SimpleSurface.h"
#include "Target.h"
#include "ofxRulr/Solvers/NormalsSurface.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Caustics {
			//----------
			SimpleSurface::SimpleSurface()
			{
				RULR_NODE_INIT_LISTENER;
				this->setIconGlyph(u8"\uf279");
			}

			//----------
			string
				SimpleSurface::getTypeName() const
			{
				return "Caustics::SimpleSurface";
			}

			//----------
			void SimpleSurface::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_UPDATE_LISTENER;

				RULR_NODE_DRAW_WORLD_LISTENER;;

				this->manageParameters(this->parameters);

				this->addInput<Target>();
			}

			//----------
			void SimpleSurface::update()
			{
				if (this->preview.dirty) {
					this->updatePreview();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				SimpleSurface::getPanel()
			{
				auto panel = ofxCvGui::Panels::makeBlank();
				return panel;
			}

			//----------
			void
				SimpleSurface::drawWorldStage()
			{
				ofPushStyle();
				ofPushMatrix();
				{
					ofScale(this->parameters.scale.get());
					ofTranslate(-0.5, -0.5, 0.0f);

					ofNoFill();
					ofDrawRectangle(0, 0, 1, 1);
				}
				ofPopMatrix();
				ofPopStyle();

				// Draw line meshes
				this->preview.directions.draw();

				// Draw surface
				{
					ofPushStyle();
					{
						ofSetColor(200);
						this->preview.surface.draw();
					}
					ofPopStyle();
				}

				// Targets
				{
					const auto& radius = this->parameters.draw.targetSize.get();
					ofPushStyle();
					{
						ofNoFill();
						ofSetColor(200, 100, 100);
						ofSetCircleResolution(4);
						for (const auto& target : this->preview.targets) {
							ofDrawCircle(target, radius);
						}
					}
					ofPopStyle();
				}
				
			}

			//----------
			void
				SimpleSurface::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Initialise", [this]() {
					try {
						this->initialise();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '1');


				inspector->addButton("Solve surface", [this]() {
					try {
						this->solveSurface();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '2');

				inspector->addButton("Integrate normals", [this]() {
					try {
						this->integrateNormals();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '3');
			}

			//----------
			void
				SimpleSurface::serialize(nlohmann::json& json)
			{
			}

			//----------
			void
				SimpleSurface::deserialize(const nlohmann::json& json)
			{
			}

			//----------
			void
				SimpleSurface::initialise()
			{
				this->throwIfMissingAConnection<Target>();
				auto target = this->getInput<Target>();

				const auto resolution = this->parameters.resolution.get();
				const auto scale = this->parameters.scale.get();

				const auto ourTransform = this->getTransform();

				this->integratedSurface.distortedGrid.initGrid(resolution, scale);

				const auto curves = target->getResampledCurves(resolution);
				auto targetPoints = target->getTargetPointsForCurves(curves);

				// All rows have same targets
				for (auto& row : this->integratedSurface.distortedGrid.positions) {
					for (size_t i = 0; i < resolution; i++) {
						row[i].target = targetPoints[i];
						row[i].normal = glm::normalize(targetPoints[i] - row[i].currentPosition);
					}
				}
				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::solveSurface()
			{
				auto surface = this->integratedSurface;
				auto result = Solvers::NormalsSurface::solve(surface
					, this->parameters.surfaceSolver.solverSettings.getSolverSettings());
				this->integratedSurface = result.solution.surface;
			}

			//----------
			void
				SimpleSurface::integrateNormals()
			{
				this->integratedSurface.integrateHeights();
				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::updatePreview()
			{
				this->preview.directions.clear();
				this->preview.directions.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				this->preview.surface.clear();
				this->preview.surface.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				this->preview.targets.clear();

				if (this->integratedSurface.distortedGrid.positions.empty()) {
					return;
				}

				auto scale = this->parameters.draw.vectorLength.get();

				for (const auto& row : this->integratedSurface.distortedGrid.positions) {
					for (const auto& position : row) {
						const auto direction = glm::normalize(position.target - position.currentPosition);

						// Add direction
						{
							{
								this->preview.directions.addVertex(position.currentPosition);
								this->preview.directions.addVertex(position.currentPosition + direction * scale);
							}

							{
								ofFloatColor color(
									ofMap(direction.x * 10, -1, 1, 0, 1)
									, ofMap(direction.y * 10, -1, 1, 0, 1)
									, ofMap(direction.z, -1, 1, 0, 1)
								);
								this->preview.directions.addColor(color);
								this->preview.directions.addColor(color);
							}
						}

						// Add target
						{
							this->preview.targets.push_back(position.target);
						}
					}
				}

				// Surface
				{
					auto getPos = [&](size_t i, size_t j) {
						return this->integratedSurface.distortedGrid.at(i, j).currentPosition;
					};

					auto rows = this->integratedSurface.distortedGrid.positions.size();
					auto cols = this->integratedSurface.distortedGrid.positions.front().size();

					for (size_t j = 0; j < rows; j++) {
						auto& row = this->integratedSurface.distortedGrid.positions[j];
						this->preview.surface.addVertex(getPos(0, j));

						for (size_t i = 1; i < row.size() - 1; i++){
							this->preview.surface.addVertex(getPos(i, j));
							this->preview.surface.addVertex(getPos(i, j));
						}

						this->preview.surface.addVertex(getPos(cols - 1, j));
					}

					for (size_t i = 0; i < cols; i++) {
						this->preview.surface.addVertex(getPos(i, 0));

						for (size_t j = 1; j < rows - 1; j++) {
							this->preview.surface.addVertex(getPos(i, j));
							this->preview.surface.addVertex(getPos(i, j));
						}

						this->preview.surface.addVertex(getPos(i, rows - 1));
					}
				}
				
				this->preview.dirty = false;

			}
		}
	}
}
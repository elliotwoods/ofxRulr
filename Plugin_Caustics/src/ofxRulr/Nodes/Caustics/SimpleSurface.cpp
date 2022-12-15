#include "pch_Plugin_Caustics.h"
#include "SimpleSurface.h"
#include "Target.h"

#include "ofxRulr/Solvers/IntegratedSurface.h"
#include "ofxRulr/Solvers/Normal.h"

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
				if (this->parameters.surfaceSolver.poissonSolver.enabled) {
					try {
						this->poissonStepHeightMap();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
				if (this->parameters.surfaceSolver.sectionSolve.continuously) {
					try {
						this->healDiscontinuities();
						for (int i = 0; i < this->parameters.surfaceSolver.sectionSolve.iterations.get(); i++) {
							this->sectionSolve();
						}
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
				if (this->preview.dirty) {
					this->updatePreview();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				SimpleSurface::getPanel()
			{
				auto panel = ofxCvGui::Panels::makeBlank();

				panel->onDraw += [this](ofxCvGui::DrawArguments& args) {
					ofPushMatrix();
					{
						const auto scale = this->parameters.scale.get();

						ofPushMatrix();
						{
							ofScale(args.localBounds.width, args.localBounds.height);
							ofTranslate(0.5, 0.5);
							ofScale(1.0f / scale);

							this->preview.surface.draw();
						}
						ofPopMatrix();
					}
					ofPopMatrix();

					ofxCvGui::Utils::drawText(ofToString(this->surface.distortedGrid.cols()) + "x" + ofToString(this->surface.distortedGrid.rows()), 10, 10);
				};
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

				if (this->parameters.draw.enabled.rays) {
					ofPushStyle();
					{
						ofSetColor(255 * this->parameters.draw.rayBrightness);
						this->preview.rays.draw();
					}
					ofPopStyle();
				}

				if (this->parameters.draw.enabled.normals) {
					this->preview.normals.draw();
				}

				if (this->parameters.draw.enabled.surface) {
					ofPushStyle();
					{
						ofSetColor(200);
						this->preview.surface.draw();
					}
					ofPopStyle();
				}

				if (this->parameters.draw.enabled.targets) {
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

				if (this->parameters.draw.enabled.residuals) {
					for (size_t i = 0; i < this->preview.residuals.size(); i++) {
						const auto & residual = this->preview.residuals[i];
						const auto & residualPosition = this->preview.residualPositions[i];
						auto hue = ofMap(abs(residual), 0.0f, this->preview.maxResidual, 270.0f, 0.0f, true);
						ofColor color(200, 100, 100);
						color.setHueAngle(hue);
						ofxCvGui::Utils::drawTextAnnotation(ofToString(residual, 2), residualPosition, color);
					}
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

				inspector->addButton("Solve normals", [this]() {
					try {
						this->solveNormals();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '2');

				inspector->addButton("Solve height map", [this]() {
					try {
						this->solveHeightMap();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '3');

				inspector->addButton("Poisson step height map", [this]() {
					try {
						this->poissonStepHeightMap();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '4');

				inspector->addButton("Combined solve step", [this]() {
					try {
						this->combinedSolveStepHeightMap();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '5');

				inspector->addButton("Section solve", [this]() {
					try {
						for (int i = 0; i < this->parameters.surfaceSolver.sectionSolve.iterations.get(); i++) {
							this->sectionSolve();
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, '6');

				inspector->addButton("Heal discontinuities", [this]() {
					try {
						this->healDiscontinuities();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, 'h');

				inspector->addButton("Pyramid up", [this]() {
					try {
						this->pyramidUp();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, 'p');

				inspector->addButton("Export height map", [this]() {
					try {
						auto result = ofSystemSaveDialog("heightmap.csv", "Save heightmap");
						if (result.bSuccess) {
							this->exportHeightMap(result.filePath);
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, 'e');
			}

			//----------
			void
				SimpleSurface::serialize(nlohmann::json& json)
			{
				this->surface.serialize(json["surface"]);
			}

			//----------
			void
				SimpleSurface::deserialize(const nlohmann::json& json)
			{
				if (json.contains("surface")) {
					this->surface.deserialize(json["surface"]);
				}
			}

			//----------
			void
				SimpleSurface::initialise()
			{
				this->throwIfMissingAConnection<Target>();

				const auto resolution = this->parameters.resolution.get();
				const auto scale = this->parameters.scale.get();

				const auto ourTransform = this->getTransform();

				this->surface.distortedGrid.initGrid(resolution, scale);

				this->calculateTargets();

				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::calculateTargets()
			{
				auto target = this->getInput<Target>();

				const auto resolution = this->parameters.resolution.get();

				const auto curves = target->getResampledCurves(resolution);
				auto targetPoints = target->getTargetPointsForCurves(curves);

				// All rows have same targets
				for (auto& row : this->surface.distortedGrid.positions) {
					for (size_t i = 0; i < resolution; i++) {
						row[i].target = targetPoints[i]; // HACK!
						row[i].incoming = glm::vec3(0, 0, 1);
					}
				}

				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::solveNormals()
			{
				for (auto& row : this->surface.distortedGrid.positions) {
					for (auto& position : row) {
						auto refracted = glm::normalize(position.target - position.currentPosition);
						auto incident = position.incoming;
						auto exitIORvsIncidentIOR = 1.0f / this->parameters.optics.materialIOR.get();
						auto result = Solvers::Normal::solve(incident
							, refracted
							, exitIORvsIncidentIOR
							, this->parameters.normalSolver.solverSettings.getSolverSettings());
						position.normal = result.solution;
					}
				}

				this->preview.dirty = true;
			}

			/*
			//----------
			void
				SimpleSurface::solveIntegratedSurface()
			{
				auto surface = this->surface;
				surface.distortedGrid.calculateDirectionVectors();

				// Bake existing transform
				for (auto& row : surface.distortedGrid.positions) {
					for (auto& position : row) {
						position.initialPosition = position.currentPosition;
					}
				}

				auto result = Solvers::IntegratedSurface::solve(surface
					, this->parameters.surfaceSolver.solverSettings.getSolverSettings());
				this->integratedSurface = result.solution.surface;
				this->preview.dirty = true;
			}
			*/

			//----------
			void
				SimpleSurface::solveHeightMap()
			{
				auto surface = this->surface;

				// Bake existing transform
				for (auto& row : surface.distortedGrid.positions) {
					for (auto& position : row) {
						position.initialPosition = position.currentPosition;
					}
				}

				if (this->parameters.surfaceSolver.universalSolve) {
					auto result = Solvers::NormalsSurface::solveUniversal(surface
						, this->parameters.surfaceSolver.solverSettings.getSolverSettings());
					this->surface = result.solution.surface;
				}
				else {
					auto result = Solvers::NormalsSurface::solveByIndividual(surface
						, this->parameters.surfaceSolver.solverSettings.getSolverSettings());
					this->surface = result.solution.surface;
				}
				
				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::poissonStepHeightMap()
			{
				for (int i = 0; i < this->parameters.surfaceSolver.poissonSolver.iterations; i++) {
					this->surface.poissonStep(this->parameters.surfaceSolver.poissonSolver.factor, false);
				}
				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::combinedSolveStepHeightMap()
			{
				auto result = Solvers::NormalsSurface::solveByIndividual(surface
					, this->parameters.surfaceSolver.solverSettings.getSolverSettings()
					, true);
				this->surface = result.solution.surface;

				this->surface.poissonStep(this->parameters.surfaceSolver.poissonSolver.factor, true);

				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::sectionSolve()
			{
				auto i_start = this->parameters.surfaceSolver.sectionSolve.i.get();
				auto j_start = this->parameters.surfaceSolver.sectionSolve.j.get();
				const auto& width = this->parameters.surfaceSolver.sectionSolve.width.get();
				const auto & overlap = this->parameters.surfaceSolver.sectionSolve.overlap.get();
				const auto rows = this->surface.distortedGrid.rows();
				const auto cols = this->surface.distortedGrid.cols();

				Models::SurfaceSectionSettings surfaceSectionSettings;
				{
					surfaceSectionSettings.i_start = i_start;
					surfaceSectionSettings.j_start = j_start;
					surfaceSectionSettings.width = width + overlap;
					surfaceSectionSettings.height = width + overlap;
				}

				// check if it overlaps edge
				if (surfaceSectionSettings.i_end() > cols) {
					surfaceSectionSettings.width -= surfaceSectionSettings.i_end() - cols;
				}
				if (surfaceSectionSettings.j_end() > rows) {
					surfaceSectionSettings.height -= surfaceSectionSettings.j_end() - rows;
				}

				// perform solve
				{
					auto result = Solvers::NormalsSurface::solveSection(surface
						, this->parameters.surfaceSolver.solverSettings.getSolverSettings()
						, surfaceSectionSettings);
					this->surface = result.solution.surface;
				}

				// Update section (so we iterate through)
				{
					auto moveVertically = this->parameters.surfaceSolver.sectionSolve.moveVertically.get();
					bool reachedEnd = false;
					if (moveVertically) {
						j_start += width;
						if (j_start >= this->surface.distortedGrid.rows()) {
							j_start = 0;
							i_start += width;
							if (i_start >= this->surface.distortedGrid.cols()) {
								i_start = 0;
								reachedEnd = true;
							}
						}
					}
					else
					{
						i_start += width;
						if (i_start >= this->surface.distortedGrid.cols()) {
							i_start = 0;
							j_start += width;
							if (j_start >= this->surface.distortedGrid.rows()) {
								j_start = 0;
								reachedEnd = true;
							}
						}
					}

					this->parameters.surfaceSolver.sectionSolve.i.set(i_start);
					this->parameters.surfaceSolver.sectionSolve.j.set(j_start);

					if (reachedEnd) {
						moveVertically ^= true;
						this->parameters.surfaceSolver.sectionSolve.moveVertically.set(moveVertically);
					}
				}

				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::healDiscontinuities()
			{
				{
					const auto maxZMagnitude = this->parameters.surfaceSolver.sectionSolve.healHeightAmplitude.get();

					for (int j = 0; j < (int) this->surface.distortedGrid.rows(); j++) {
						for (int i = 0; i < (int) this->surface.distortedGrid.cols(); i++) {
							
							auto& ourPosition = this->surface.distortedGrid.at(i, j);
							const auto ourHeight = ourPosition.getHeight();

							//First take mean of out distance to neighboring heights to see if we are a problem
							{
								float accumulate = 0.0f;
								float count = 0.0f;
								{
									auto addToMean = [&](int _i, int _j) {
										if (this->surface.distortedGrid.inside(_i, _j)) {
											count++;
											accumulate += abs(this->surface.distortedGrid.at(_i, _j).getHeight() - ourHeight);
										}
									};
									//Perform on local grid
									for (int _j = j - 1; _j < j + 1; _j++) {
										for (int _i = i - 1; _i < i + 1; _i++) {
											addToMean(_i, _j);
										}
									}
								}

								auto meanDistance = accumulate / count;

								// Check if discontinuity found
								if (meanDistance >  maxZMagnitude) {
									this->healAround(i, j);
								}
							}
						}
					}
				}

				this->preview.dirty = true;
			}
			//----------
			void
				SimpleSurface::healAround(size_t i, size_t j)
			{
				// To do a heal we:
				// 1. zero any big vertices
				// 2. solve around the region

				// Setup the region
				Models::SurfaceSectionSettings surfaceSectionSettings;
				const auto& width = (int)this->parameters.surfaceSolver.sectionSolve.width.get();
				auto i_start = (int)i - width / 2;
				auto j_start = (int)j - width / 2; 
				{

					if (i_start < 0) {
						i_start = 0;
					}
					if (j_start < 0) {
						j_start = 0;
					}

					const auto rows = this->surface.distortedGrid.rows();
					const auto cols = this->surface.distortedGrid.cols();

					{
						surfaceSectionSettings.i_start = i_start;
						surfaceSectionSettings.j_start = j_start;
						surfaceSectionSettings.width = width;
						surfaceSectionSettings.height = width;
					}

					// check if it overlaps edge
					if (surfaceSectionSettings.i_end() > cols) {
						surfaceSectionSettings.width -= surfaceSectionSettings.i_end() - cols;
					}
					if (surfaceSectionSettings.j_end() > rows) {
						surfaceSectionSettings.height -= surfaceSectionSettings.j_end() - rows;
					}
				}

				// 1. Zero any large heights
				{
					const auto maxHeightAbs = this->parameters.surfaceSolver.sectionSolve.healHeightAmplitude.get();

					for (size_t j = surfaceSectionSettings.j_start; j < surfaceSectionSettings.j_end(); j++) {
						for (size_t i = surfaceSectionSettings.i_start; i < surfaceSectionSettings.i_end(); i++) {
							auto& position = this->surface.distortedGrid.at(i, j);
							if (abs(position.getHeight()) > maxHeightAbs) {
								position.setHeight(0);
							}
						}
					}
				}

				// 2. Local region solve
				{
					// perform solve
					{
						auto result = Solvers::NormalsSurface::solveSection(this->surface
							, this->parameters.surfaceSolver.solverSettings.getSolverSettings()
							, surfaceSectionSettings);
						this->surface = result.solution.surface;
					}
				}
				

				this->preview.dirty = true;
			}

			//----------
			void
				SimpleSurface::pyramidUp()
			{
				// upscale the surface
				this->surface.distortedGrid = this->surface.distortedGrid.pyramidUp();

				// apply the target positions
				this->calculateTargets();
				
				// calculate new normals
				this->solveNormals();
			}
			//----------
			void
				SimpleSurface::updatePreview()
			{
				this->preview.rays.clear();
				this->preview.rays.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				this->preview.normals.clear();
				this->preview.normals.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				this->preview.surface.clear();
				this->preview.surface.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				this->preview.targets.clear();

				if (this->surface.distortedGrid.positions.empty()) {
					return;
				}

				auto scale = this->parameters.draw.vectorLength.get();

				for (const auto& row : this->surface.distortedGrid.positions) {
					for (const auto& position : row) {
						const auto direction = glm::normalize(position.target - position.currentPosition);

						// Add normal
						{
							{
								this->preview.normals.addVertex(position.currentPosition);
								this->preview.normals.addVertex(position.currentPosition + position.normal * scale);
							}

							{
								ofFloatColor color(
									ofMap(position.normal.x * 10, -1, 1, 0, 1)
									, ofMap(position.normal.y * 10, -1, 1, 0, 1)
									, ofMap(position.normal.z, -1, 1, 0, 1)
								);
								this->preview.normals.addColor(color);
								this->preview.normals.addColor(color);
							}
						}

						// Add rays incoming
						{
							this->preview.rays.addVertex(position.currentPosition);
							this->preview.rays.addVertex(position.currentPosition - position.incoming * scale);
						}

						// Add rays outgoing
						{
							this->preview.rays.addVertex(position.currentPosition);
							this->preview.rays.addVertex(position.currentPosition + direction * scale);
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
						return this->surface.distortedGrid.at(i, j).currentPosition;
					};

					auto rows = this->surface.distortedGrid.positions.size();
					auto cols = this->surface.distortedGrid.positions.front().size();

					for (size_t j = 0; j < rows; j++) {
						auto& row = this->surface.distortedGrid.positions[j];
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

			//----------
			void
				SimpleSurface::exportHeightMap(const std::filesystem::path& path) const
			{
				ofFile file(path, ofFile::Mode::WriteOnly, false);
				for (const auto& row : this->surface.distortedGrid.positions) {
					for (auto it = row.begin(); it != row.end(); it++) {
						if (it != row.begin()) {
							file << ", ";
						}
						file << it->currentPosition.z;
					}
					file << std::endl;
				}
				file.close();
			}
		}
	}
}
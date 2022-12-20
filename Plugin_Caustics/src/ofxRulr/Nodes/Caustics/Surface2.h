#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Models/IntegratedSurface.h"
#include "ofxRulr/Solvers/Normal.h"
#include "ofxRulr/Solvers/IntegratedSurface.h"
#include "ofxRulr/Solvers/NormalsSurface.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Caustics {
			class SimpleSurface : public ofxRulr::Nodes::Item::RigidBody {
			public:
				SimpleSurface();
				string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel();

				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				void initialise();
				void calculateTargets();
				void solveNormals();
				void solveHeightMap();
				void poissonStepHeightMap();
				void combinedSolveStepHeightMap();
				void sectionSolve();
				void healDiscontinuities();
				void healAround(size_t i, size_t j);

				void pyramidUp();

				void updatePreview();

				Models::Surface& getSurface();
				void setSurface(const Models::Surface&);

				void buildSolidMesh();
				void exportHeightMap(const std::filesystem::path&) const;
				void exportMesh(const std::filesystem::path&) const;

			protected:
				struct : ofParameterGroup {
					ofParameter<float> scale{ "Scale", 1.0f, 0.0f, 10.0f };
					ofParameter<int> resolution{ "Resolution", 256 };

					struct : ofParameterGroup {
						ofParameter<float> materialIOR{ "Material IOR", 1.5304, 1, 2 };
						PARAM_DECLARE("Optics", materialIOR);
					} optics;

					struct : ofParameterGroup {
						ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::NormalsSurface::getDefaultSolverSettings() };
						ofParameter<bool> universalSolve{ "Universal solve", false };

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", false };
							ofParameter<float> factor{ "Factor", 1.5 };
							ofParameter<int> iterations{ "Iterations", 1 };
							PARAM_DECLARE("Poisson", enabled, factor, iterations);
						} poissonSolver;

						struct : ofParameterGroup {
							ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::NormalsSurface::getDefaultSolverSettings() };
							ofParameter<size_t> i{ "i", 0 };
							ofParameter<size_t> j{ "j", 0 };
							ofParameter<size_t> width{ "Width", 8 };
							ofParameter<size_t> overlap{ "Overlap", 0 };
							ofParameter<bool> continuously{ "Continuously", false };
							ofParameter<float> healHeightAmplitude{ "Heal height amp", 0.1, 0, 2 };
							ofParameter<bool> moveVertically{ "Move vertically", false };
							ofParameter<int> iterations{ "Iterations", 1 };
							PARAM_DECLARE("Section solve", solverSettings, i, j, width, overlap, continuously, healHeightAmplitude, moveVertically, iterations);
						} sectionSolve;

						PARAM_DECLARE("Surface solver", solverSettings, universalSolve, poissonSolver, sectionSolve);
					} surfaceSolver;

					struct : ofParameterGroup {
						ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::Normal::getDefaultSolverSettings() };
						PARAM_DECLARE("Normal solver", solverSettings);
					} normalSolver;

					struct : ofParameterGroup {
						ofParameter<float> vectorLength{ "Vector length", 0.1, 0, 0.1 };
						ofParameter<float> targetSize{ "Target size", 0.02, 0, 0.1 };
						ofParameter<float> rayBrightness{ "Ray brightness", 0.3, 0, 1 };

						struct : ofParameterGroup {
							ofParameter<bool> targets{ "Targets", true };
							ofParameter<bool> rays{ "Rays", true };
							ofParameter<bool> normals{ "Normals", true };
							ofParameter<bool> surface{ "Surface", true };
							ofParameter<bool> residuals{ "Resisuals", false };
							ofParameter<bool> solidMesh{ "Solid mesh", false };
							PARAM_DECLARE("Enabled", targets, rays, normals, surface, residuals, solidMesh);
						} enabled;

						PARAM_DECLARE("Draw", vectorLength, targetSize, rayBrightness, enabled)
					} draw;

					struct : ofParameterGroup {
						ofParameter<float> backFaceZ{ "Back face Z", -0.01, -1, 1 };
						PARAM_DECLARE("Mesh", backFaceZ);
					} mesh;

					PARAM_DECLARE("SimpleSurface"
						, scale
						, resolution
						, optics
						, surfaceSolver
						, normalSolver
						, draw
						, mesh);
				} parameters;

				Models::Surface surface;

				struct {
					bool dirty = true;
					ofMesh rays;
					ofMesh normals;
					ofMesh surface;
					vector<glm::vec3> targets;

					vector<float> residuals;
					vector<glm::vec3> residualPositions;
					float maxResidual = 0.0f;

					struct {
						ofLight left;
						ofLight top;
						ofLight front;
					} lighting;
				} preview;

				ofMesh solidMesh;
			};
		}
	}
}
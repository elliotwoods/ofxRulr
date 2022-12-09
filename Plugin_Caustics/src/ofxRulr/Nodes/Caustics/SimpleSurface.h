#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"
#include "ofxRulr/Models/IntegratedSurface.h"
#include "ofxRulr/Solvers/Normal.h"
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
				void solveNormals();
				void solveSurfaceDistortion();
				void integrateNormals();

				void updatePreview();
			protected:
				struct : ofParameterGroup {
					ofParameter<float> scale{ "Scale", 1.0f, 0.0f, 10.0f };
					ofParameter<int> resolution{ "Resolution", 256 };

					struct : ofParameterGroup {
						ofParameter<float> materialIOR{ "Material IOR", 1.5304, 1, 2 };
						PARAM_DECLARE("Optics", materialIOR);
					} optics;

					struct : ofParameterGroup {
						ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::Normal::getDefaultSolverSettings() };
						PARAM_DECLARE("Normals solver", solverSettings);
					} normalsSolver;

					struct : ofParameterGroup {
						ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::NormalsSurface::getDefaultSolverSettings() };
						PARAM_DECLARE("Surface solver", solverSettings);
					} surfaceSolver;

					struct : ofParameterGroup {
						ofParameter<float> vectorLength{ "Vector length", 0.1, 0, 0.1 };
						ofParameter<float> targetSize{ "Target size", 0.02, 0, 0.1 };
						ofParameter<float> rayBrightness{ "Ray brightness", 0.3, 0, 1 };
						PARAM_DECLARE("Draw", vectorLength, targetSize, rayBrightness)
					} draw;

					PARAM_DECLARE("SimpleSurface"
						, scale
						, resolution
						, optics
						, normalsSolver
						, surfaceSolver
						, draw);
				} parameters;

				Models::IntegratedSurface integratedSurface;

				struct {
					bool dirty = true;
					ofMesh directions;
					ofMesh surface;
					vector<glm::vec3> targets;
				} preview;
			};
		}
	}
}
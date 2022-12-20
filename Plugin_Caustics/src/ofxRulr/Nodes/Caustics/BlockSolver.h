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
			class BlockSolver : public ofxRulr::Nodes::Base {
			public:
				BlockSolver();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				void solve();
				void reset();
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::NormalsSurface::getDefaultSolverSettings() };
						ofParameter<size_t> i{ "i", 0 };
						ofParameter<size_t> j{ "j", 0 };
						ofParameter<size_t> width{ "Width", 8 };
						ofParameter<size_t> height{ "Height", 8 };
						ofParameter<bool> continuously{ "Continuously", false };
						ofParameter<bool> once{ "Once", true };
						ofParameter<bool> move{ "Move", false };
						ofParameter<float> healHeightAmplitude{ "Heal height amp", 0.1, 0, 2 };
						ofParameter<int> iterations{ "Iterations", 1 };
						PARAM_DECLARE("Section solve", solverSettings, i, j, width, height, continuously, once, move, healHeightAmplitude, iterations);
					} sectionSolve;
					PARAM_DECLARE("BlockSolver", sectionSolve);
				} parameters;
			};
		}
	}
}
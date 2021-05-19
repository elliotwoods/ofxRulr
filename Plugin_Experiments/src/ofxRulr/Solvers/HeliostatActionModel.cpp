#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"


namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			HeliostatActionModel::Navigator::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.max_num_iterations = 1000;
			return solverSettings;
		}

	}
}
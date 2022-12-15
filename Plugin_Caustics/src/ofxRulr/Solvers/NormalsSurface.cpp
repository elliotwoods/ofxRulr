#include "pch_Plugin_Caustics.h"
#include "NormalsSurface.h"

struct NormalsSurfaceCost
{
	//----------
	NormalsSurfaceCost(const ofxRulr::Models::Surface_<double>& priorSurface)
		: priorSurface(priorSurface)
	{

	}

	//----------
	template<typename T>
	bool
		operator()(const T* const* heightParameters
			, T* residuals) const
	{
		auto surface = this->priorSurface.castTo<T>();
		surface.fromParameters(heightParameters[0]);
		surface.getResiduals(residuals);
		return true;
	}

	//----------
	static ceres::CostFunction*
		Create(const ofxRulr::Models::Surface_<double>& priorSurface)
	{
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<NormalsSurfaceCost, 4>(
			new NormalsSurfaceCost(priorSurface)
			);
		costFunction->AddParameterBlock(priorSurface.getParameterCount());
		costFunction->SetNumResiduals(priorSurface.getResidualCount());
		return costFunction;
	}

	ofxRulr::Models::Surface_<double> priorSurface;
};

struct SingleNormalsSurfaceCost
{
	//----------
	SingleNormalsSurfaceCost(const ofxRulr::Models::Surface_<double>& surface
		, size_t i
		, size_t j
		, size_t cols)
		: priorSurface(surface)
		, i(i)
		, j(j)
		, cols(cols)
	{

	}

	//----------
	template<typename T>
	bool
		operator()(const T* const* heightParameters
			, T* residuals) const
	{
		auto estimatedNormal = this->priorSurface.estimateIndividualNormal(heightParameters[0]
			, this->i
			, this->j
			, this->cols);
		const auto& intendedNormal = (glm::tvec3<T>) this->priorSurface.distortedGrid.at(this->i, this->j).normal;
		auto delta = intendedNormal - estimatedNormal;
		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];
		return true;
	}

	//----------
	static ceres::CostFunction*
		Create(const ofxRulr::Models::Surface_<double>& surface
		, size_t i
		, size_t j
		, size_t cols)
	{
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<SingleNormalsSurfaceCost, 4>(
			new SingleNormalsSurfaceCost(surface, i, j, cols)
			);
		costFunction->AddParameterBlock(surface.getParameterCount());
		costFunction->SetNumResiduals(3);
		return costFunction;
	}

	const ofxRulr::Models::Surface_<double> & priorSurface;
	size_t i;
	size_t j;
	size_t cols;
};

struct SingleNormalsSurfaceSectionCost
{
	//----------
	SingleNormalsSurfaceSectionCost(const ofxRulr::Models::Surface_<double>& surface
		, size_t i
		, size_t j
		, const ofxRulr::Models::SurfaceSectionSettings& sectionSettings)
		: priorSurface(surface)
		, i(i)
		, j(j)
		, sectionSettings(sectionSettings)
	{

	}

	//----------
	template<typename T>
	bool
		operator()(const T* const* heightParameters
			, T* residuals) const
	{
		ofxRulr::Models::HeightSectionParameters_<T> heightSectionParameters(this->sectionSettings, heightParameters);

		auto estimatedNormal = this->priorSurface.estimateIndividualNormalWithinSection(heightSectionParameters
			, this->i
			, this->j);
		const auto& intendedNormal = (glm::tvec3<T>) this->priorSurface.distortedGrid.at(this->i, this->j).normal;
		auto delta = intendedNormal - estimatedNormal;
		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];
		return true;
	}

	//----------
	static ceres::CostFunction*
		Create(const ofxRulr::Models::Surface_<double>& surface
			, size_t i
			, size_t j
			, const ofxRulr::Models::SurfaceSectionSettings& sectionSettings)
	{
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<SingleNormalsSurfaceSectionCost, 4>(
			new SingleNormalsSurfaceSectionCost(surface, i, j, sectionSettings)
			);

		// calculate parameter count
		for (size_t _j = 0; _j < sectionSettings.height; _j++) {
			costFunction->AddParameterBlock(sectionSettings.width);
		}

		costFunction->SetNumResiduals(3);
		return costFunction;
	}

	const ofxRulr::Models::Surface_<double>& priorSurface;
	size_t i;
	size_t j;
	ofxRulr::Models::SurfaceSectionSettings sectionSettings;
};


namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			NormalsSurface::getDefaultSolverSettings()
		{
			auto solverSettings = ofxCeres::SolverSettings();
			//solverSettings.options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
			return solverSettings;
		}

		//----------
		vector<double*>
			NormalsSurface::initParameters(const Models::Surface_<double>& surface)
		{
			vector<double*> allParameters;

			// Grid parameters
			{
				const auto parametersCount = surface.getParameterCount();
				auto parameters = new double[parametersCount];
				surface.toParameters(parameters);
				allParameters.push_back(parameters);
			}

			return allParameters;
		}

		//----------
		NormalsSurface::Result
			NormalsSurface::solveUniversal(const Models::Surface& initialCondition
				, const ofxCeres::SolverSettings& solverSettings)
		{
			if (initialCondition.distortedGrid.positions.empty()) {
				throw(ofxCeres::Exception("Grid is empty"));
			}

			ceres::Problem problem;

			// Create the surface
			auto surface = initialCondition.castTo<double>();

			// Create parameters
			auto allParameters = NormalsSurface::initParameters(surface);

			// Add the problem block
			problem.AddResidualBlock(NormalsSurfaceCost::Create(surface)
				, NULL
				, allParameters);

			// Perform the solve
			ceres::Solver::Summary summary;
			{
				if (solverSettings.printReport) {
					cout << "Solve OpticalSystemSolver" << endl;
				}
				ceres::Solve(solverSettings.options
					, &problem
					, &summary);

				if (solverSettings.printReport) {
					cout << summary.FullReport() << endl;
				}
			}

			// Bring parameters back
			{
				surface.fromParameters(allParameters[0]);
			}

			// Destroy parameter blocks
			{
				for (const auto& parameterBlock : allParameters) {
					delete[] parameterBlock;
				}
			}

			// Create the result
			{
				Result result(summary);
				result.solution.surface = surface.castTo<float>();
				return result;
			}
		}

		//----------
		NormalsSurface::Result
			NormalsSurface::solveByIndividual(const Models::Surface& initialCondition
				, const ofxCeres::SolverSettings& solverSettings
				, bool edgesOnly)
		{
			if (initialCondition.distortedGrid.positions.empty()) {
				throw(ofxCeres::Exception("Grid is empty"));
			}

			ceres::Problem problem;

			// Create the surface
			auto surface = initialCondition.castTo<double>();

			// Create parameters
			auto allParameters = NormalsSurface::initParameters(surface);

			// Add the problem block
			const auto cols = surface.distortedGrid.cols();
			const auto rows = surface.distortedGrid.rows();
			for (size_t j = 0; j < rows; j++) {
				for (size_t i = 0; i < cols; i++) {
					if (edgesOnly) {
						auto isEdge = i == 0 || i == cols - 1
							|| j == 0 || j == rows - 1;
						if (!isEdge) {
							continue;
						}
					}

					problem.AddResidualBlock(SingleNormalsSurfaceCost::Create(surface
						, i
						, j
						, cols)
						, NULL
						, allParameters);
				}
			}

			// Perform the solve
			ceres::Solver::Summary summary;
			{
				if (solverSettings.printReport) {
					cout << "Solve OpticalSystemSolver" << endl;
				}
				ceres::Solve(solverSettings.options
					, &problem
					, &summary);

				if (solverSettings.printReport) {
					cout << summary.FullReport() << endl;
				}
			}

			// Bring parameters back
			{
				surface.fromParameters(allParameters[0]);
			}

			// Destroy parameter blocks
			{
				for (const auto& parameterBlock : allParameters) {
					delete[] parameterBlock;
				}
			}

			// Create the result
			{
				Result result(summary);
				result.solution.surface = surface.castTo<float>();
				return result;
			}
		}

		//----------
		NormalsSurface::Result
			NormalsSurface::solveSection(const Models::Surface& initialCondition
				, const ofxCeres::SolverSettings& solverSettings
				, const Models::SurfaceSectionSettings & surfaceSectionSettings)
		{
			if (initialCondition.distortedGrid.positions.empty()) {
				throw(ofxCeres::Exception("Grid is empty"));
			}

			ceres::Problem problem;

			// Create the surface
			auto surface = initialCondition.castTo<double>();

			// Check surface section settings
			if (surfaceSectionSettings.i_end() > surface.distortedGrid.cols()
				|| surfaceSectionSettings.j_end() > surface.distortedGrid.rows()) {
				throw(ofxRulr::Exception("Surface section overlaps edge of grid"));
			}

			// Create parameters
			auto allParameters = surfaceSectionSettings.initParameters(surface.distortedGrid);

			// Add the problem block
			const auto cols = surface.distortedGrid.cols();
			const auto rows = surface.distortedGrid.rows();

			{
				auto i_start = surfaceSectionSettings.i_start;
				auto j_start = surfaceSectionSettings.j_start;
				auto i_end = min(rows, surfaceSectionSettings.i_end());
				auto j_end = min(cols, surfaceSectionSettings.j_end());

				// erode the solve section where not at boundary
				if (i_start != 0) {
					i_start++;
				}
				if (i_end != cols) {
					i_end--;
				}
				if (j_start != 0) {
					j_start++;
				}
				if (j_end != rows) {
					j_end--;
				}

				for (size_t j = j_start; j < j_end; j++) {
					for (size_t i = i_start; i < i_end; i++) {
						problem.AddResidualBlock(SingleNormalsSurfaceSectionCost::Create(surface
							, i
							, j
							, surfaceSectionSettings)
							, NULL
							, allParameters);
					}
				}
			}
			

			// Perform the solve
			ceres::Solver::Summary summary;
			{
				if (solverSettings.printReport) {
					cout << "Solve OpticalSystemSolver" << endl;
				}
				ceres::Solve(solverSettings.options
					, &problem
					, &summary);

				if (solverSettings.printReport) {
					cout << summary.FullReport() << endl;
				}
			}

			// Bring parameters back
			{
				surfaceSectionSettings.applyParameters(surface.distortedGrid
					, allParameters);
			}

			// Destroy parameter blocks
			{
				for (const auto& parameterBlock : allParameters) {
					delete[] parameterBlock;
				}
			}

			// Create the result
			{
				Result result(summary);
				result.solution.surface = surface.castTo<float>();
				return result;
			}
		}
	}
}

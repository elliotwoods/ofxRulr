#include "pch_Plugin_Scrap.h"
#include "BundleAdjustmentLasers.h"

struct ProjectedLineCost
{
	//---------
	ProjectedLineCost(const glm::vec2& projectionPoint
		, const ofxRulr::Models::Line& imageLine
		, const ofxRulr::Models::Intrinsics& cameraIntrinsics
		, const float weight)
		: projectionPoint(projectionPoint)
		, imageLine(imageLine)
		, cameraIntrinsics(cameraIntrinsics)
		, weight(weight)
	{

	}

	//---------
	template<typename T>
	bool
		operator()(const T* const cameraViewTranslationParameters
			, const T* const cameraViewRotationParameters
			, const T* const laserRigidBodyTranslationParameters
			, const T* const laserRigidBodyRotationParameters
			, const T* const laserFovParameters
			, T* residuals) const
	{
		// Construct the camera
		const auto castIntrinsics = this->cameraIntrinsics.castTo<T>();
		const auto viewTransform = ofxRulr::Models::Transform_<T>(cameraViewTranslationParameters
			, cameraViewRotationParameters);
		ofxRulr::Models::Camera_<T> camera(viewTransform, castIntrinsics);

		// Construct the LaserProjector
		ofxRulr::Models::LaserProjector_<T> laserProjector {
			ofxRulr::Models::Transform_<T>(laserRigidBodyTranslationParameters
				, laserRigidBodyRotationParameters)
			, {
				laserFovParameters[0]
				, laserFovParameters[1]
			}
		};

		// Cast the line
		auto imageLine = this->imageLine.castTo<T>();

		// Residual of observed line vs world position of laser
		{
			// Project the laser position into camera space
			auto laserProjectorInCamera = camera.worldToImage(laserProjector.rigidBodyTransform.translation);

			// Delta the projected camera vs observed line as residual
			residuals[0] = imageLine.distanceToPoint(laserProjectorInCamera);
		}

		// Residual of observed line vs ray in camera
		{
			// Project the laser projectionPoint into world space Ray
			const auto ray = laserProjector.castRayWorldSpace((glm::tvec2<T>) this->projectionPoint);

			// Project the laser Ray into a Line camera image space
			const auto line = camera.worldToImage(ray);

			// Take a point on the line close to the center of camera image
			const auto imageCenter = (glm::tvec2<T>) this->cameraIntrinsics.getCenter();
			const auto pointInImage = line.getClosestPointTo(imageCenter);

			// Delta the point vs observed line as residual
			residuals[1] = imageLine.distanceToPoint(pointInImage);
		}

		return true;
	}

	//---------
	static ceres::CostFunction*
		Create(const glm::vec2& projectionPoint
			, const ofxRulr::Models::Line& imageLine
			, const ofxRulr::Models::Intrinsics& cameraIntrinsics
			, const float weight)
	{
		return new ceres::AutoDiffCostFunction<ProjectedLineCost, 2, 3, 3, 3, 3, 3>(
			new ProjectedLineCost(projectionPoint, imageLine, cameraIntrinsics, weight)
			);
	}

	const glm::vec2 projectionPoint;
	const ofxRulr::Models::Line imageLine;
	const ofxRulr::Models::Intrinsics& cameraIntrinsics; // this is stored in Problem
	const float weight;
};


namespace ofxRulr {
	namespace Solvers {
		//----------
		void
			BundleAdjustmentLasers::fillCameraParameters(const Models::Transform& cameraViewTransform
			, double* cameraTranslationParameters
			, double* cameraRotationParameters)
		{
			{
				cameraTranslationParameters[0] = (double)cameraViewTransform.translation[0];
				cameraTranslationParameters[1] = (double)cameraViewTransform.translation[1];
				cameraTranslationParameters[2] = (double)cameraViewTransform.translation[2];
			}

			{
				cameraRotationParameters[0] = (double)cameraViewTransform.rotation[0];
				cameraRotationParameters[1] = (double)cameraViewTransform.rotation[1];
				cameraRotationParameters[2] = (double)cameraViewTransform.rotation[2];
			}
		}

		//----------
		void
			BundleAdjustmentLasers::fillLaserParameters(const Models::LaserProjector& laserProjector
				, double* laserTranslationParameters
				, double* laserRotationParameters
				, double* laserFovParameters)
		{
			{
				laserTranslationParameters[0] = laserProjector.rigidBodyTransform.translation[0];
				laserTranslationParameters[1] = laserProjector.rigidBodyTransform.translation[1];
				laserTranslationParameters[2] = laserProjector.rigidBodyTransform.translation[2];
			}

			{
				laserRotationParameters[0] = laserProjector.rigidBodyTransform.rotation[0];
				laserRotationParameters[1] = laserProjector.rigidBodyTransform.rotation[1];
				laserRotationParameters[2] = laserProjector.rigidBodyTransform.rotation[2];
			}

			{
				laserFovParameters[0] = laserProjector.fov[0];
				laserFovParameters[1] = laserProjector.fov[1];
			}
		}

		//----------
		ofxCeres::SolverSettings
			BundleAdjustmentLasers::defaultSolverSettings()
		{
			// These are copied from MarkerProjections

			ofxCeres::SolverSettings solverSettings;
			solverSettings.printReport = true;
			solverSettings.options.max_num_iterations = 10000;
			solverSettings.options.function_tolerance = 1e-8;

			// according to http://ceres-solver.org/solving_faqs.html
			solverSettings.options.linear_solver_type = ceres::LinearSolverType::DENSE_SCHUR;

			return solverSettings;
		}

		//----------
		BundleAdjustmentLasers::Problem::Problem(const Solution& initialSolution
			, const Models::Intrinsics& cameraIntrinsics)
			: cameraIntrinsics(cameraIntrinsics)
		{
			for(const auto& cameraViewTransform : initialSolution.cameraViewTransforms) {
				auto cameraTranslationParameters = new double[3];
				auto cameraRotationParameters = new double[3];

				fillCameraParameters(cameraViewTransform
					, cameraTranslationParameters
					, cameraRotationParameters);

				this->allCameraTranslationParameters.push_back(cameraTranslationParameters);
				this->allCameraRotationParameters.push_back(cameraRotationParameters);
			}

			for (const auto& laserProjector : initialSolution.laserProjectors) {
				auto laserTranslationParameters = new double[3];
				auto laserRotationParameters = new double[3];
				auto laserFovParameters = new double[2];

				fillLaserParameters(laserProjector
					, laserTranslationParameters
					, laserRotationParameters
					, laserFovParameters);

				this->allLaserTranslationParameters.push_back(laserTranslationParameters);
				this->allLaserRotationParameters.push_back(laserRotationParameters);
				this->allLaserFovParameters.push_back(laserFovParameters);
			}
		}

		//----------
		BundleAdjustmentLasers::Problem::~Problem()
		{
			for (auto parameters : this->allCameraTranslationParameters) {
				delete[] parameters;
			}

			for (auto parameters : this->allCameraRotationParameters) {
				delete[] parameters;
			}

			for (auto parameters : this->allLaserTranslationParameters) {
				delete[] parameters;
			}

			for (auto parameters : this->allLaserRotationParameters) {
				delete[] parameters;
			}

			for (auto parameters : this->allLaserFovParameters) {
				delete[] parameters;
			}
		}

		//----------
		void
			BundleAdjustmentLasers::Problem::addLineImageObservation(const Image& image)
		{
			if (image.cameraIndex >= this->allCameraTranslationParameters.size()) {
				throw(ofxRulr::Exception("image.cameraIndex is out of range"));
			}

			if (image.laserProjectorIndex >= this->allLaserTranslationParameters.size()) {
				throw(ofxRulr::Exception("image.laserProjectorIndex is out of range"));
			}

			auto& cameraViewTranslationData = this->allCameraTranslationParameters[image.cameraIndex];
			auto& cameraViewRotationData = this->allCameraRotationParameters[image.cameraIndex];

			auto& laserRigidBodyTranslationData = this->allLaserTranslationParameters[image.laserProjectorIndex];
			auto& laserRigidBodyRotationData = this->allLaserRotationParameters[image.laserProjectorIndex];
			auto& laserFovData = this->allLaserFovParameters[image.laserProjectorIndex];

			const float weight = 1.0f;

			auto residualBlock = ProjectedLineCost::Create(image.projectedPoint
				, image.imageLine
				, this->cameraIntrinsics
				, weight);

			this->problem.AddResidualBlock(residualBlock
				, NULL
				, cameraViewTranslationData
				, cameraViewRotationData
				, laserRigidBodyTranslationData
				, laserRigidBodyRotationData
				, laserFovData);
		}

		//----------
		BundleAdjustmentLasers::Result
			BundleAdjustmentLasers::Problem::solve(const ofxCeres::SolverSettings& solverSettings)
		{
			if (solverSettings.printReport) {
				cout << "Solve BundleAdjustmentLasers" << endl;
			}
			ceres::Solver::Summary summary;
			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				cout << summary.FullReport() << endl;
			}

			{
				Result result(summary);

				{
					auto cameraCount = this->allCameraTranslationParameters.size();
					for (size_t i = 0; i < cameraCount; i++) {
						auto cameraViewTranslationParameters = this->allCameraTranslationParameters[i];
						auto cameraViewRotationParameters = this->allCameraRotationParameters[i];
						auto transform = Models::Transform_<double>(cameraViewTranslationParameters, cameraViewRotationParameters).castTo<float>();
						result.solution.cameraViewTransforms.push_back(transform);
					}
				}

				{
					auto laserCount = this->allLaserTranslationParameters.size();
					for (size_t i = 0; i < laserCount; i++) {
						auto laserRigidBodyTranslationParameters = this->allLaserTranslationParameters[i];
						auto laserRigidBodyRotationParameters = this->allLaserRotationParameters[i];
						auto transform = Models::Transform_<double>(laserRigidBodyTranslationParameters, laserRigidBodyRotationParameters).castTo<float>();

						auto laserFovParameters = this->allLaserFovParameters[i];
						auto fov = (glm::vec2)glm::tvec2<double>(laserFovParameters[0], laserFovParameters[1]);
						result.solution.laserProjectors.emplace_back(Models::LaserProjector{ transform, fov });
					}
				}

				return result;
			}
		}

		//----------
		float
			BundleAdjustmentLasers::getResidual(const Solution& solution
				, const Models::Intrinsics& cameraIntrinsics
				, const Image& image)
		{
			double cameraViewTranslationParameters[3];
			double cameraViewRotationParameters[3];
			double laserTranslationParameters[3];
			double laserRotationParameters[3];
			double laserFovParameters[2];

			// Fill the parameters
			{
				fillCameraParameters(solution.cameraViewTransforms[image.cameraIndex]
					, cameraViewTranslationParameters
					, cameraViewRotationParameters);
				fillLaserParameters(solution.laserProjectors[image.laserProjectorIndex]
					, laserTranslationParameters
					, laserRotationParameters
					, laserFovParameters);
			}

			// Create cost function
			ProjectedLineCost costFunction (image.projectedPoint
				, image.imageLine
				, cameraIntrinsics
				, 1.0f);

			// Calculate residuals
			double residuals[2];
			if (!costFunction(cameraViewTranslationParameters
				, cameraViewRotationParameters
				, laserTranslationParameters
				, laserRotationParameters
				, laserFovParameters
				, residuals)) {
				throw(ofxRulr::Exception("BundleAdjustmentLasers::getResidual : Failed to evaluate cost function"));
			}

			// Take RMS
			float rms = sqrt((residuals[0] * residuals[0] + residuals[1] * residuals[1]) / 2.0f);
			return rms;
		}
	}
}
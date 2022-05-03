#include "pch_Plugin_Scrap.h"
#include "BundleAdjustmentPoints.h"

#include "ofxRulr/Models/Camera.h"

struct ProjectedPointCost
{
	//---------
	ProjectedPointCost(const glm::vec2& imagePoint
		, const ofxRulr::Models::Intrinsics& cameraIntrinsics
		, const float weight)
		: imagePoint(imagePoint)
		, cameraIntrinsics(cameraIntrinsics)
		, weight(weight)
	{

	}

	//---------
	template<typename T>
	bool
		operator()(const T* const worldPointParameters
			, const T* const cameraViewTranslationParameters
			, const T* const cameraViewRotationParameters
			, T* residuals) const
	{
		// Construct the camera
		const auto castIntrinsics = this->cameraIntrinsics.castTo<T>();
		const auto viewTransform = ofxRulr::Models::Transform_<T>(cameraViewTranslationParameters, cameraViewRotationParameters);
		ofxRulr::Models::Camera_<T> camera(viewTransform, castIntrinsics);

		// Construct the world point
		glm::tvec3<T> worldPoint(worldPointParameters[0]
			, worldPointParameters[1]
			, worldPointParameters[2]);

		// Project the world point into image space
		auto projectedImagePoint = camera.worldToImage(worldPoint);

		// Get the delta
		auto delta = projectedImagePoint - (glm::tvec2<T>) this->imagePoint;

		// Give the residuals
		residuals[0] = delta[0] * (T) this->weight;
		residuals[1] = delta[1] * (T) this->weight;

		return true;
	}

	//---------
	static ceres::CostFunction*
		Create(const glm::vec2& imagePoint
			, const ofxRulr::Models::Intrinsics& cameraIntrinsics
			, const float& weight)
	{
		return new ceres::AutoDiffCostFunction<ProjectedPointCost, 2, 3, 3, 3>(
			new ProjectedPointCost(imagePoint, cameraIntrinsics, weight)
			);
	}

	const glm::vec2 imagePoint;
	const ofxRulr::Models::Intrinsics & cameraIntrinsics; // this is stored in Problem
	const float weight;
};

struct SceneRadiusCost
{
	//---------
	SceneRadiusCost(const float& targetMaxRadius
		, size_t pointCount)
		: targetMaxRadius(targetMaxRadius)
		, pointCount(pointCount)
	{

	}

	//---------
	template<typename T>
	bool
		operator()(T const * const * parameters
			, T* residuals) const
	{
		// Calculate mean
		glm::tvec3<T> meanPosition;
		for (size_t i = 0; i < this->pointCount; i++) {
			const auto& point = *(glm::tvec3<T>*) parameters[i];
			meanPosition += point;
		}
		meanPosition /= (T)this->pointCount;

		// Calculate max radius
		T maxRadius = (T)0;
		for (size_t i = 0; i < this->pointCount; i++) {
			const auto& point = *(glm::tvec3<T>*) parameters[i];
			auto radius = ofxCeres::VectorMath::distance(meanPosition, point);
			if (radius > maxRadius) {
				maxRadius = radius;
			}
		}
		
		// Residual is log(maxRadius / targetMaxRadius). i.e. = 0 when they are equal
		residuals[0] = log(maxRadius / (T) this->targetMaxRadius);

		return true;
	}

	//---------
	static ceres::CostFunction*
		Create(const float & targetMaxRadius, size_t pointCount)
	{
		// Make a dynamic cost function
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<SceneRadiusCost, 4>(
			new SceneRadiusCost(targetMaxRadius, pointCount)
			);
		for (size_t i = 0; i < pointCount; i++) {
			costFunction->AddParameterBlock(3);
		}
		costFunction->SetNumResiduals(1);
		return costFunction;
	}

	const float targetMaxRadius;
	const size_t pointCount;
};

struct SceneCenterCost
{
	//---------
	SceneCenterCost(const glm::vec3& sceneCenter
		, size_t pointCount)
		: sceneCenter(sceneCenter)
		, pointCount(pointCount)
	{

	}

	//---------
	template<typename T>
	bool
		operator()(T const* const* parameters
			, T* residuals) const
	{
		// Calculate scene center
		glm::tvec3<T> accumulator((T) 0, (T) 0, (T) 0);
		for (size_t i = 0; i < this->pointCount; i++) {
			const auto& point = *(glm::tvec3<T>*) parameters[i];
			accumulator = accumulator + point;
		}
		auto mean = accumulator / (T)this->pointCount;

		auto delta = mean - (glm::tvec3<T>) this->sceneCenter;

		// Residual is 3D
		residuals[0] = delta[0];
		residuals[1] = delta[1];
		residuals[2] = delta[2];

		return true;
	}

	//---------
	static ceres::CostFunction*
		Create(const glm::vec3& sceneCenter
			, size_t pointCount)
	{
		// Make a dynamic cost function
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<SceneCenterCost, 4>(
			new SceneCenterCost(sceneCenter, pointCount)
			);
		for (size_t i = 0; i < pointCount; i++) {
			costFunction->AddParameterBlock(3);
		}
		costFunction->SetNumResiduals(3);
		return costFunction;
	}

	const glm::vec3 sceneCenter;
	const size_t pointCount;
};

struct CameraFixedRotateCost
{
	//---------
	CameraFixedRotateCost(const size_t& axis
		, const float& angle)
		: axis(axis)
		, angle(angle)
	{

	}

	//---------
	template<typename T>
	bool
		operator()(const T* const cameraViewRotationParameters
			, T* residuals) const
	{
		// Aiming for specific euler angle
		auto delta = cameraViewRotationParameters[this->axis] - (T) this->angle;
		residuals[0] = delta;

		return true;
	}

	//---------
	static ceres::CostFunction*
		Create(const size_t& axis
			, const float& angle)
	{
		return new ceres::AutoDiffCostFunction<CameraFixedRotateCost, 1, 3>(
			new CameraFixedRotateCost(axis, angle)
			);
	}

	const size_t axis;
	const size_t angle;
};

struct PointsInPlaneCost
{
	//---------
	PointsInPlaneCost(const size_t& plane
		, const size_t& pointCount)
		: plane(plane)
		, pointCount(pointCount)
	{

	}

	//---------
	template<typename T>
	bool
		operator()(T const* const* worldPointParameters
			, T* residuals) const
	{
		// Calculate scene center
		glm::tvec3<T> accumulator((T)0, (T)0, (T)0);
		for (size_t i = 0; i < this->pointCount; i++) {
			const auto& point = *(glm::tvec3<T>*) worldPointParameters[i];
			accumulator = accumulator + point;
		}
		auto mean = accumulator / (T)this->pointCount;

		// Give the residuals
		for (size_t i = 0; i < pointCount; i++) {
			residuals[i] = worldPointParameters[i][this->plane] - mean[this->plane];
		}

		return true;
	}

	//---------
	static ceres::CostFunction*
		Create(const size_t& plane
			, const size_t& pointCount)
	{
		// Make a dynamic cost function
		auto costFunction = new ceres::DynamicAutoDiffCostFunction<PointsInPlaneCost, 4>(
			new PointsInPlaneCost(plane, pointCount)
			);
		for (size_t i = 0; i < pointCount; i++) {
			costFunction->AddParameterBlock(3);
		}
		costFunction->SetNumResiduals(pointCount);
		return costFunction;
	}

	const size_t plane;
	const size_t pointCount;
};

namespace ofxRulr {
	namespace Solvers {
		//---------
		ofxCeres::SolverSettings
			BundleAdjustmentPoints::defaultSolverSettings()
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

		//---------
		BundleAdjustmentPoints::Problem::Problem(size_t pointCount
			, size_t viewCount
			, const Models::Intrinsics& cameraIntrinsics)
			: cameraIntrinsics(cameraIntrinsics)
		{
			for (size_t i = 0; i < pointCount; i++) {
				this->allWorldPointParameters.push_back(new double[3]);
			}

			for (size_t i = 0; i < viewCount; i++) {
				this->allViewTranslateParameters.push_back(new double[3]);
				this->allViewRotationParameters.push_back(new double[3]);
			}
		}

		//---------
		BundleAdjustmentPoints::Problem::Problem(const Solution& initialSolution
			, const Models::Intrinsics& cameraIntrinsics)
			: cameraIntrinsics(cameraIntrinsics)
		{
			for (const auto& worldPoint : initialSolution.worldPoints) {
				auto data = new double[3];
				data[0] = (double)worldPoint[0];
				data[1] = (double)worldPoint[1];
				data[2] = (double)worldPoint[2];
				this->allWorldPointParameters.push_back(data);
			}

			for (const auto& cameraView : initialSolution.cameraViewTransforms) {
				{
					auto data = new double[3];
					data[0] = (double)cameraView.translation[0];
					data[1] = (double)cameraView.translation[1];
					data[2] = (double)cameraView.translation[2];
					this->allViewTranslateParameters.push_back(data);
				}

				{
					auto data = new double[3];
					data[0] = (double)cameraView.rotation[0];
					data[1] = (double)cameraView.rotation[1];
					data[2] = (double)cameraView.rotation[2];
					this->allViewRotationParameters.push_back(data);
				}
			}
		}

		//---------
		BundleAdjustmentPoints::Problem::~Problem()
		{
			for (auto& data : allWorldPointParameters) {
				delete[] data;
			}
			for (auto& data : allViewTranslateParameters) {
				delete[] data;
			}
			for (auto& data : allViewRotationParameters) {
				delete[] data;
			}
		}

		//---------
		void
		BundleAdjustmentPoints::Problem::addImagePointObservation(const Image& image
			, bool applyWeightByDistanceFromImageCenter)
		{
			if (image.pointIndex >= this->allWorldPointParameters.size()) {
				throw(ofxRulr::Exception("image.pointIndex is out of range"));
			}

			if (image.viewIndex >= this->allViewTranslateParameters.size()) {
				throw(ofxRulr::Exception("image.viewIndex is out of range"));
			}

			auto& worldPointData = this->allWorldPointParameters[image.pointIndex];
			auto& viewTranslationData = this->allViewTranslateParameters[image.viewIndex];
			auto& viewRotationData = this->allViewRotationParameters[image.viewIndex];

			float weight = 1.0f;
			if (applyWeightByDistanceFromImageCenter) {
				auto distanceToImageCenter = image.imagePoint - glm::vec2(this->cameraIntrinsics.width, this->cameraIntrinsics.height);
				weight = 1.0f / sqrt(glm::length(distanceToImageCenter));
			}
			auto residualBlock = ProjectedPointCost::Create(image.imagePoint
				, this->cameraIntrinsics
				, weight);

			this->problem.AddResidualBlock(residualBlock
				, NULL
				, worldPointData
				, viewTranslationData
				, viewRotationData);
		}

		//---------
		void
			BundleAdjustmentPoints::Problem::addSceneScaleConstraint(float maxRadius)
		{
			auto residualBlock = SceneRadiusCost::Create(maxRadius, this->allWorldPointParameters.size());
			this->problem.AddResidualBlock(residualBlock
				, NULL
				, this->allWorldPointParameters);
		}

		//---------
		void
			BundleAdjustmentPoints::Problem::addSceneCenteredConstraint(const glm::vec3& sceneCenter)
		{
			auto residualBlock = SceneCenterCost::Create(sceneCenter, this->allWorldPointParameters.size());
			this->problem.AddResidualBlock(residualBlock
				, NULL
				, this->allWorldPointParameters);
		}

		//---------
		void
			BundleAdjustmentPoints::Problem::addCameraZeroYawConstraint(int viewIndex)
		{
			if (viewIndex >= this->allViewRotationParameters.size()) {
				throw(ofxRulr::Exception("viewIndex is out of range"));
			}

			auto residualBlock = CameraFixedRotateCost::Create(1, 0.0f);
			this->problem.AddResidualBlock(residualBlock
				, NULL
				, this->allViewRotationParameters[viewIndex]);
		}

		//---------
		void
			BundleAdjustmentPoints::Problem::addCameraZeroRollConstrant(int viewIndex)
		{
			if (viewIndex >= this->allViewRotationParameters.size()) {
				throw(ofxRulr::Exception("viewIndex is out of range"));
			}

			auto residualBlock = CameraFixedRotateCost::Create(2, 0.0f);
			this->problem.AddResidualBlock(residualBlock
				, NULL
				, this->allViewRotationParameters[viewIndex]);
		}

		//---------
		void
			BundleAdjustmentPoints::Problem::addPointsInPlaneConstraint(size_t plane)
		{
			auto residualBlock = PointsInPlaneCost::Create(plane, this->allWorldPointParameters.size());
			this->problem.AddResidualBlock(residualBlock
				, NULL
				, this->allWorldPointParameters);
			
		}

		//---------
		BundleAdjustmentPoints::Result
			BundleAdjustmentPoints::Problem::solve(const ofxCeres::SolverSettings& solverSettings)
		{
			if (solverSettings.printReport) {
				cout << "Solve BundleAdjustmentPoints" << endl;
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

				for (auto& worldPointParameters : this->allWorldPointParameters) {
					result.solution.worldPoints.push_back((glm::vec3) * (glm::tvec3<double>*) worldPointParameters);
				}

				auto viewCount = this->allViewTranslateParameters.size();
				for (size_t i = 0; i < viewCount; i++) {
					auto viewTranslationParameters = this->allViewTranslateParameters[i];
					auto viewRotationParameters = this->allViewRotationParameters[i];
					auto transform = Models::Transform_<double>(viewTranslationParameters, viewRotationParameters).castTo<float>();
					result.solution.cameraViewTransforms.push_back(transform);
				}

				return result;
			}

		}
	}
}

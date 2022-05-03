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
			for (const auto& cameraView : initialSolution.cameraViewTransforms) {
				{
					auto data = new double[3];
					data[0] = (double)cameraView.translation[0];
					data[1] = (double)cameraView.translation[1];
					data[2] = (double)cameraView.translation[2];
					this->allCameraTranslationParameters.push_back(data);
				}

				{
					auto data = new double[3];
					data[0] = (double)cameraView.rotation[0];
					data[1] = (double)cameraView.rotation[1];
					data[2] = (double)cameraView.rotation[2];
					this->allCameraRotationParameters.push_back(data);
				}
			}

			for (const auto& laserProjector : initialSolution.laserProjectors) {
				{
					const auto& rigidBodyTransform = laserProjector.rigidBodyTransform;

					{
						auto data = new double[3];
						data[0] = (double)rigidBodyTransform.translation[0];
						data[1] = (double)rigidBodyTransform.translation[1];
						data[2] = (double)rigidBodyTransform.translation[2];
						this->allLaserTranslationParameters.push_back(data);
					}

					{
						auto data = new double[3];
						data[0] = (double)rigidBodyTransform.rotation[0];
						data[1] = (double)rigidBodyTransform.rotation[1];
						data[2] = (double)rigidBodyTransform.rotation[2];
						this->allLaserRotationParameters.push_back(data);
					}
				}

				{
					auto data = new double[2];
					data[0] = (double)laserProjector.fov[0];
					data[1] = (double)laserProjector.fov[1];
					this->allLaserFovParameters.push_back(data);
				}
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
				, laserRigidBodyRotationData);
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
	}
}
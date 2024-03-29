#include "pch_Plugin_Experiments.h"
#include "HeliostatActionModel.h"

using namespace ofxCeres::VectorMath;
using namespace ofxRulr::Solvers;

struct HeliostatActionModel_CalibratorRayCost
{
	HeliostatActionModel_CalibratorRayCost(const ofxRay::Ray & cameraRay
		, const glm::vec3 & worldPoint
		, const float axis1ServoPosition
		, const float axis2ServoPosition
		, const float axis1AngleOffset
		, const float axis2AngleOffset
		, const float mirrorDiameter)

		: cameraRay(cameraRay)
		, worldPoint(worldPoint)
		, axis1ServoPosition(axis1ServoPosition)
		, axis2ServoPosition(axis2ServoPosition)
		, axis1AngleOffset(axis1AngleOffset)
		, axis2AngleOffset(axis2AngleOffset)
		, mirrorDiameter(mirrorDiameter)
	{

	}

	template<typename T>
	bool
		operator()(const T* const positionParameters
			, const T* const rotationYParameters
			, const T* const rotationAxisParameters
			, const T* const polynomialParameters
			, const T* const mirrorOffsetParameters
			, T* residuals) const
	{
		// Construct the HAM::Parameters
		HeliostatActionModel::Parameters<T> hamParameters;
		hamParameters.fromParameterStrings(positionParameters
			, rotationYParameters
			, rotationAxisParameters
			, polynomialParameters
			, mirrorOffsetParameters);

		// Construct the camera ray
		ofxCeres::Models::Ray<T> cameraRay;
		cameraRay.s = glm::tvec3<T>(this->cameraRay.s);
		cameraRay.t = glm::tvec3<T>(this->cameraRay.t);

		// Calculate axis angles (apply polynomial)
		HeliostatActionModel::AxisAngles<T> axisAngles{
			axisAngles.axis1 = HeliostatActionModel::positionToAngle<T>((T)this->axis1ServoPosition
				, hamParameters.axis1.polynomial
				, (T)this->axis1AngleOffset)
			, axisAngles.axis2 = HeliostatActionModel::positionToAngle<T>((T)this->axis2ServoPosition
				, hamParameters.axis2.polynomial
				, (T)this->axis2AngleOffset)
		};

		// Get the mirror plane
		auto mirrorPlane = HeliostatActionModel::getMirrorPlane<T>(axisAngles, hamParameters);
		auto reflectedRay = mirrorPlane.reflect(cameraRay);

		// First residual is world distance between reflected ray and target point
		{
			auto distance = reflectedRay.distanceTo(worldPoint);
			residuals[0] = distance * distance;
		}

		// Calculate the distance off the mirror face
		{
			auto intersection = mirrorPlane.intersect(cameraRay);
			auto intersectionRadius = distance(mirrorPlane.center, intersection);
			auto mirrorRadius = (T)this->mirrorDiameter / (T)2.0;
			if (intersectionRadius > mirrorRadius) {
				residuals[1] = (intersectionRadius - mirrorRadius) * (T) 1000.0;
			}
			else {
				residuals[1] = (T)0.0;
			}
		}

		// Add a residual if reflected ray is facing away from the mirror normal
		{
			auto alignment = dot(normalize(reflectedRay.t), mirrorPlane.normal);
			if (alignment < (T) 0) {
				residuals[2] = -alignment * (T) 1000.0;
			}
			else {
				residuals[2] = (T)0.0;
			}
		}

		return true;
	}

	static ceres::CostFunction*
		Create(const ofxRay::Ray& cameraRay
			, const glm::vec3& worldPoint
			, const float axis1ServoPosition
			, const float axis2ServoPosition
			, const float axis1AngleOffset
			, const float axis2AngleOffset
			, const float mirrorDiameter)
	{
		return new ceres::AutoDiffCostFunction<HeliostatActionModel_CalibratorRayCost, 3, 3, 1, 4, 6, 1>(
			new HeliostatActionModel_CalibratorRayCost(cameraRay
				, worldPoint
				, axis1ServoPosition
				, axis2ServoPosition
				, axis1AngleOffset
				, axis2AngleOffset
				, mirrorDiameter)
			);
	}

	const ofxRay::Ray & cameraRay;
	const glm::vec3 & worldPoint;
	const float axis1ServoPosition;
	const float axis2ServoPosition;
	const float mirrorDiameter;
	const float axis1AngleOffset;
	const float axis2AngleOffset;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			HeliostatActionModel::Calibrator::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			solverSettings.options.max_num_iterations = 1000;
			solverSettings.options.parameter_tolerance = 1e-7;
			solverSettings.options.function_tolerance = 1e-8;
			solverSettings.options.minimizer_progress_to_stdout = true;
			solverSettings.printReport = true;
			return solverSettings;
		}

		//----------
		HeliostatActionModel::Calibrator::Result
			HeliostatActionModel::Calibrator::solveCalibration(const vector<ofxRay::Ray>& cameraRays
				, const vector<glm::vec3>& worldPoints
				, const vector<int>& axis1ServoPosition
				, const vector<int>& axis2ServoPosition
				, const vector<float>& axis1AngleOffset
				, const vector<float>& axis2AngleOffset
				, const HeliostatActionModel::Parameters<float>& priorParameters
				, const Options& options
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Check length of data
			{
				auto size = cameraRays.size();
				if (size != worldPoints.size()
					|| size != axis1ServoPosition.size()
					|| size != axis2ServoPosition.size()) {
					throw(ofxRulr::Exception("Size mismatch"));
				}
			}

			// Initialise parameters
			double positionParameters[3];
			double rotationYParameters[1];
			double rotationAxisParameters[4];
			double polynomialParameters[6];
			double mirrorOffsetParameters[1];
			priorParameters.castTo<double>().toParameterStrings(positionParameters
				, rotationYParameters
				, rotationAxisParameters
				, polynomialParameters
				, mirrorOffsetParameters);

			// Construct one problem per ray
			ceres::Problem problem;
			{
				for (size_t i = 0; i < cameraRays.size(); i++) {
					{
						auto costFunction = HeliostatActionModel_CalibratorRayCost::Create(cameraRays[i]
							, worldPoints[i]
							, (float)axis1ServoPosition[i]
							, (float)axis2ServoPosition[i]
							, axis1AngleOffset[i]
							, axis2AngleOffset[i]
							, options.mirrorDiameter);
						problem.AddResidualBlock(costFunction
							, NULL
							, positionParameters
							, rotationYParameters
							, rotationAxisParameters
							, polynomialParameters
							, mirrorOffsetParameters
						);
					}
				}
			}

			// Apply the options
			{
				if (options.fixPosition) {
					problem.SetParameterBlockConstant(positionParameters);
				}
				
				if (options.fixRotationY) {
					problem.SetParameterBlockConstant(rotationYParameters);
				}
				
				if (options.fixRotationAxis) {
					problem.SetParameterBlockConstant(rotationAxisParameters);
				}
				
				if (options.fixMirrorOffset) {
					problem.SetParameterBlockConstant(mirrorOffsetParameters);
				}
				else {
					problem.SetParameterLowerBound(mirrorOffsetParameters, 0, 0.13);
					problem.SetParameterUpperBound(mirrorOffsetParameters, 0, 0.15);
				}

				if (options.fixPolynomial) {
					problem.SetParameterBlockConstant(polynomialParameters);
				}
				else {
					problem.SetParameterLowerBound(polynomialParameters, 0, -128);
					problem.SetParameterUpperBound(polynomialParameters, 0, 128);
					problem.SetParameterLowerBound(polynomialParameters, 1, 0.9);
					problem.SetParameterUpperBound(polynomialParameters, 1, 1.1);
					problem.SetParameterLowerBound(polynomialParameters, 2, -1e-5);
					problem.SetParameterUpperBound(polynomialParameters, 2, 1e-5);

					problem.SetParameterLowerBound(polynomialParameters, 3, -512);
					problem.SetParameterUpperBound(polynomialParameters, 3, 512);
					problem.SetParameterLowerBound(polynomialParameters, 4, 0.9);
					problem.SetParameterUpperBound(polynomialParameters, 4, 1.1);
					problem.SetParameterLowerBound(polynomialParameters, 5, -1e-5);
					problem.SetParameterUpperBound(polynomialParameters, 5, 1e-5);
				}
			}

			// Solve the fit
			ceres::Solver::Summary summary;
			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				std::cout << summary.FullReport() << "\n";
			}

			// Build the result
			HeliostatActionModel::Calibrator::Result result(summary);
			{
				Parameters<double> resultDouble;
				resultDouble.fromParameterStrings(positionParameters
					, rotationYParameters
					, rotationAxisParameters
					, polynomialParameters
					, mirrorOffsetParameters);
				result.solution = resultDouble.castTo<float>();
			}

			return result;
		}

		//----------
		float HeliostatActionModel::Calibrator::getResidual(const ofxRay::Ray& cameraRay
			, const glm::vec3& worldPoint
			, int axis1ServoPosition
			, int axis2ServoPosition
			, float axis1AngleOffset
			, float axis2AngleOffset
			, const Parameters<float>& hamParameters
			, float mirrorDiameter) {

			// Initialise cost function
			auto costFunction = HeliostatActionModel_CalibratorRayCost(cameraRay
				, worldPoint
				, axis1ServoPosition
				, axis2ServoPosition
				, axis1AngleOffset
				, axis2AngleOffset
				, mirrorDiameter);

			// Initialise parameters
			double positionParameters[3];
			double rotationYParameters[1];
			double rotationAxisParameters[4];
			double polynomialParameters[6];
			double mirrorOffsetParameters[1];
			hamParameters.castTo<double>().toParameterStrings(positionParameters
				, rotationYParameters
				, rotationAxisParameters
				, polynomialParameters
				, mirrorOffsetParameters);

			double residuals[3];

			costFunction(positionParameters
				, rotationYParameters
				, rotationAxisParameters
				, polynomialParameters
				, mirrorOffsetParameters
				, residuals);

			return (float)residuals[0];
		}
	}
}
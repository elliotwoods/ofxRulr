#include "pch_Plugin_Experiments.h"
#include "MirrorPlanefromRays.h"

using namespace ofxCeres::VectorMath;

struct MirrorPlaneFromRaysCost
{
	MirrorPlaneFromRaysCost(const ofxRay::Ray& cameraRay
		, const glm::vec3 & worldPoint)
		: cameraRay(cameraRay)
		, worldPoint(worldPoint)
	{

	}
	template<typename T>
	bool
		operator()(const T* const parameters
			, T* residuals) const
	{
		// construct the plane
		const auto planeNormal = glm::tvec3<T>(parameters[0], parameters[1], parameters[2]);
		const auto planeD = parameters[3];

		// construct the camera ray
		const auto cameraRayS = glm::tvec3<T>(this->cameraRay.s);
		const auto cameraRayT = glm::tvec3<T>(this->cameraRay.t);

		// intersect the ray and the plane
		// page 20 of ReMarkable notes May 2021
		const T u = -(planeD + dot(cameraRayS, planeNormal))
			/ dot(cameraRayT, planeNormal);
		auto intersection = cameraRayS + u * cameraRayT;
		
		// reflected ray starts with intersection between cameraRay and mirror plane
		const auto& reflectedRayS = intersection;

		// reflected ray transmission is defined by reflect(cameraRayT + reflectedRayS, mirrorPlane)
		auto reflectedRayT = reflect(cameraRayT + reflectedRayS, planeNormal, planeD) - reflectedRayS;
		
		// distance between reflected ray and worldPoint
		// reference from ofxRay::Ray::distanceTo
		// also https://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
		{
			const auto& x0 = (glm::tvec3<T>) worldPoint;
			const auto& x1 = reflectedRayS;
			const auto& x2 = reflectedRayS + reflectedRayT;
			residuals[0] = length(cross(x0 - x1, x0 - x2))
				/ length(x2 - x1);
		}

		return true;
	}

	static ceres::CostFunction*
		Create(const ofxRay::Ray & cameraRay
			, const glm::vec3 & worldPoint)
	{
		return new ceres::AutoDiffCostFunction<MirrorPlaneFromRaysCost, 1, 4>(
			new MirrorPlaneFromRaysCost(cameraRay, worldPoint)
			);
	}

	const ofxRay::Ray & cameraRay;
	const glm::vec3 & worldPoint;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			MirrorPlaneFromRays::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			return solverSettings;
		}

		//----------
		MirrorPlaneFromRays::Result
			MirrorPlaneFromRays::solve(const vector<ofxRay::Ray>& cameraRays
				, const vector<glm::vec3>& worldPoints
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Check same length vectors
			if (cameraRays.size() != worldPoints.size()) {
				throw(ofxRulr::Exception("MirrorPlaneFromRays : input data length mismatch"));
			}
			if (cameraRays.empty()) {
				throw(ofxRulr::Exception("MirrorPlaneFromRays : input data empty"));
			}

			// Initialise parameters
			double parameters[4];
			{
				// Initialise with a plane facing towards the first camera ray
				ofxRay::Plane initialPlane(cameraRays.front().getEnd(), -cameraRays.front().t);
				auto initialParameters = initialPlane.getABCD();
				parameters[0] = initialParameters[0];
				parameters[1] = initialParameters[1];
				parameters[2] = initialParameters[2];
				parameters[3] = initialParameters[3];
			}

			ceres::Problem problem;

			// Add all the correspondences
			{
				for (size_t i = 0; i < cameraRays.size(); i++) {
					auto costFunction = MirrorPlaneFromRaysCost::Create(cameraRays[i], worldPoints[i]);
					problem.AddResidualBlock(costFunction
						, NULL
						, parameters);
				}
			}

			ceres::Solver::Summary summary;

			ceres::Solve(solverSettings.options
				, &problem
				, &summary);

			if (solverSettings.printReport) {
				std::cout << summary.FullReport() << "\n";
			}

			ofxRay::Plane plane((float)parameters[0], (float)parameters[1], (float)parameters[2], (float)parameters[3]);

			plane.setUp({ 0.0, 1.0, 0.0 });
			plane.setScale({ 0.2, 0.2 });
			plane.setInfinite(false);

			//flip the plane if it points away from the worldPoints
			{
				if (glm::dot(plane.getNormal(), worldPoints.front() - plane.getCenter()) < 0.0f) {
					plane.setNormal(-plane.getNormal());
				}
			}

			//move the plane to be centered on the reflections
			{
				glm::vec3 accumulator;
				for (const auto& cameraRay : cameraRays) {
					glm::vec3 position;
					plane.intersect(cameraRay, position);
					accumulator += position;
				}
				plane.setCenter(accumulator / (float)cameraRays.size());
			}

			{
				Result result(summary);
				result.solution.plane = plane;
				return result;
			}
		}
	
		//----------
		float MirrorPlaneFromRays::getResidual(const glm::vec4& planeABCD
			, const ofxRay::Ray& cameraRay
			, const glm::vec3 worldPoint) {
			auto costFunction = MirrorPlaneFromRaysCost(cameraRay, worldPoint);
			
			float residual;
			costFunction(&planeABCD[0], &residual);
			return residual;
		}
	}
}
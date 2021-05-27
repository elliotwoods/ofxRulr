#include "pch_Plugin_ArUco.h"
#include "MarkerProjections.h"
#include <glm/gtx/matrix_decompose.hpp>

using namespace ofxCeres::VectorMath;

template<typename T>
class Transform {
public:
	glm::tvec3<T> rotation;
	glm::tvec3<T> translation;

	Transform(const T* const parameters) {
		this->rotation[0] = parameters[0];
		this->rotation[1] = parameters[1];
		this->rotation[2] = parameters[2];
		this->translation[0] = parameters[3];
		this->translation[1] = parameters[4];
		this->translation[2] = parameters[5];
	}

	glm::tmat4x4<T> getTransform() {
		return ofxCeres::VectorMath::createTransform(this->translation, this->rotation);
	}
};

struct MarkerProjection_Cost
{
	MarkerProjection_Cost(int cameraWidth
		, int cameraHeight
		, const glm::mat4& cameraProjectionMatrix
		, const vector<glm::vec2> imagePoints
		, const vector<glm::vec3> objectPoints)
		: cameraWidth(cameraWidth)
		, cameraHeight(cameraHeight)
		, cameraProjectionMatrix(cameraProjectionMatrix)
		, imagePoints(imagePoints)
		, objectPoints(objectPoints)
	{

	}

	template<typename T>
	bool
		operator()(const T* const viewParameters
			, const T* const objectParameters
			, T* residuals) const
	{
		// Now we need to make the residual
		auto viewTransform = Transform<T>(viewParameters);
		auto objectTransform = Transform<T>(objectParameters);

		auto cameraProjectionMatrix = (glm::tmat4x4<T>)this->cameraProjectionMatrix;

		for (int i = 0; i < this->objectPoints.size(); i++) {
			auto objectPoint = (glm::tvec3<T>) this->objectPoints[i];
			auto worldPoint = ofxCeres::VectorMath::applyTransform(objectTransform.getTransform(), objectPoint);
			auto viewPoint = ofxCeres::VectorMath::applyTransform(viewTransform.getTransform(), worldPoint);
			auto projectedPoint = ofxCeres::VectorMath::applyTransform(cameraProjectionMatrix, viewPoint);
			glm::tvec2<T> projectedImagePoint{
				(T)this->cameraWidth * (projectedPoint.x + (T)1.0) / (T)2.0
				, (T)this->cameraHeight * ((T)1.0 - projectedPoint.y) / (T)2.0
			};
			auto actualImagePoint = (glm::tvec2<T>) this->imagePoints[i];
			residuals[i] = glm::distance2(projectedImagePoint, actualImagePoint);
		}

		return true;
	}

	static ceres::CostFunction*
		Create(int cameraWidth
			, int cameraHeight
			, const glm::mat4& cameraProjectionMatrix
			, const vector<glm::vec2> imagePoints
			, const vector<glm::vec3> objectPoints)
	{
		if (imagePoints.size() != objectPoints.size()) {
			throw(ofxRulr::Exception("imagePoints.size() != objectPoints.size()"));
		}
		return new ceres::AutoDiffCostFunction<MarkerProjection_Cost, 4, 6, 6>(
			new MarkerProjection_Cost(cameraWidth, cameraHeight, cameraProjectionMatrix, imagePoints, objectPoints)
			);
	}

	int cameraWidth;
	int cameraHeight;
	const glm::mat4& cameraProjectionMatrix;
	const vector<glm::vec2> imagePoints;
	const vector<glm::vec3> objectPoints;
};

namespace ofxRulr {
	namespace Solvers {
		//----------
		ofxCeres::SolverSettings
			MarkerProjections::defaultSolverSettings()
		{
			ofxCeres::SolverSettings solverSettings;
			return solverSettings;
		}

		//----------
		MarkerProjections::Result
			MarkerProjections::solve(int cameraWidth
				, int cameraHeight
				, const glm::mat4& cameraProjectionMatrix
				, const std::vector<vector<glm::vec3>>& objectPoints
				, const vector<Image>& images
				, const Solution& initialSolution
				, const ofxCeres::SolverSettings& solverSettings)
		{
			// Check the incoming data
			{
				// Check all objects seen once
				{
					set<int> allObjects;
					{
						for (int i = 0; i < objectPoints.size(); i++) {
							allObjects.insert(i);
						}
					}
					auto unseenObjects = allObjects;

					for (const auto& image : images) {
						if (allObjects.find(image.objectIndex) == allObjects.end()) {
							throw(ofxRulr::Exception("Object index [" + ofToString(image.objectIndex) + "] outside of range"));
						}
						if (unseenObjects.find(image.objectIndex) != unseenObjects.end()) {
							unseenObjects.erase(image.objectIndex);
						}
					}

					if (!unseenObjects.empty()) {
						stringstream unseenObjectIndices;
						for (auto unseenObject : unseenObjects) {
							unseenObjectIndices << unseenObject << " ";
						}

						throw(ofxRulr::Exception("Images do not contain objects : " + unseenObjectIndices.str()));
					}

					if (initialSolution.objects.size() != objectPoints.size()) {
						throw(ofxRulr::Exception("Incoming initialised objects count != objectPoints.size()"));
					}
				}

				// Check image count matches initialisation data
				{
					set<int> allViews;
					{
						for (int i = 0; i < initialSolution.views.size(); i++) {
							allViews.insert(i);
						}
					}
					auto unseenViews = allViews;

					for (const auto& image : images) {
						if (allViews.find(image.viewIndex) == allViews.end()) {
							throw(ofxRulr::Exception("View index [" + ofToString(image.viewIndex) + "] outside of range"));
						}
						if (unseenViews.find(image.viewIndex) != unseenViews.end()) {
							unseenViews.erase(image.viewIndex);
						}
					}

					if (!unseenViews.empty()) {
						stringstream unseenViewIndices;
						for (auto unseenView : unseenViews) {
							unseenViewIndices << unseenView << " ";
						}

						throw(ofxRulr::Exception("Images do not contain views : " + unseenViewIndices.str()));
					}
				}
			}


			// Initialise parameters
			vector<double*> allParameters;
			vector<double*> allViewParameters;
			vector<double*> allObjectParameters;
			{
				for (const auto& view : initialSolution.views) {
					auto viewParameters = new double[6];
					viewParameters[0] = view.rotation[0];
					viewParameters[1] = view.rotation[1];
					viewParameters[2] = view.rotation[2];
					viewParameters[3] = view.translation[0];
					viewParameters[4] = view.translation[1];
					viewParameters[5] = view.translation[2];
					allViewParameters.push_back(viewParameters);
					allParameters.push_back(allViewParameters.back());
				}

				for (const auto& object : initialSolution.objects) {
					auto objectParameters = new double[6];
					objectParameters[0] = object.rotation[0];
					objectParameters[1] = object.rotation[1];
					objectParameters[2] = object.rotation[2];
					objectParameters[3] = object.translation[0];
					objectParameters[4] = object.translation[1];
					objectParameters[5] = object.translation[2];
					allObjectParameters.push_back(objectParameters);
					allParameters.push_back(allObjectParameters.back());
				}
			}

			// Construct the problem
			ceres::Problem problem;
			{
				for (const auto& image : images) {
					auto costFunction = MarkerProjection_Cost::Create(cameraWidth
						, cameraHeight
						, cameraProjectionMatrix
						, image.imagePoints
						, objectPoints[image.objectIndex]);
					problem.AddResidualBlock(costFunction
						, NULL
						, allViewParameters[image.viewIndex]
						, allObjectParameters[image.objectIndex]);
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
			MarkerProjections::Result result(summary);
			{
				// Views
				for (int i = 0; allViewParameters.size(); i++) {
					auto transformParameters = allViewParameters[i];
					Solution::Transform transform;
					transform.rotation[0] = transformParameters[0];
					transform.rotation[1] = transformParameters[1];
					transform.rotation[2] = transformParameters[2];
					transform.translation[0] = transformParameters[3];
					transform.translation[1] = transformParameters[4];
					transform.translation[2] = transformParameters[5];
					result.solution.views.push_back(transform);
				}

				// Objects
				for (int i = 0; allObjectParameters.size(); i++) {
					auto transformParameters = allObjectParameters[i];
					Solution::Transform transform;
					transform.rotation[0] = transformParameters[0];
					transform.rotation[1] = transformParameters[1];
					transform.rotation[2] = transformParameters[2];
					transform.translation[0] = transformParameters[3];
					transform.translation[1] = transformParameters[4];
					transform.translation[2] = transformParameters[5];
					result.solution.objects.push_back(transform);
				}
			}

			// Delete the parameters
			for (auto parameters : allParameters) {
				delete[] parameters;
			}

			return result;
		}

		//----------
		MarkerProjections::Solution::Transform
			MarkerProjections::getTransform(const glm::mat4& matrix)
		{
			glm::vec3 scale;
			glm::quat orientation;
			glm::vec3 translation, skew;
			glm::vec4 perspective;

			glm::decompose(matrix
				, scale
				, orientation
				, translation
				, skew
				, perspective);

			MarkerProjections::Solution::Transform transform;
			transform.translation = translation;
			transform.rotation = glm::eulerAngles(orientation);
			
			return transform;
		}

		//----------
		glm::mat4
			MarkerProjections::getTransform(const Solution::Transform& transform)
		{
			return ofxCeres::VectorMath::createTransform(transform.translation, transform.rotation);
		}
	}
}
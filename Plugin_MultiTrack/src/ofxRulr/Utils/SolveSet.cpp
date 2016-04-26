#include "pch_MultiTrack.h"
#include "SolveSet.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		SolveSet::SolveSet() {
			clear();
		}

		//----------
		SolveSet::~SolveSet() {
			clear();
		}

		//----------
		void SolveSet::clear() {
			this->completed = false;
			this->result.success = false;

			this->srcPoints.clear();
			this->dstPoints.clear();
		}

		//----------
		void SolveSet::setup(const vector<ofVec3f> & srcPoints, const vector<ofVec3f> & dstPoints) {
			clear();

			if (srcPoints.size() != dstPoints.size()) {
				ofLogError("SolveSet") << "Source and destination sets mismatch!";
				return;
			}

			//Copy the data.
			this->srcPoints = srcPoints;
			this->dstPoints = dstPoints;
		}

		//----------
		void SolveSet::solveNLOpt() {
			ofxNonLinearFit::Models::RigidBody::DataSet dataSet;

			//Copy the data.
			for (size_t i = 0; i < srcPoints.size(); ++i) {
				ofxNonLinearFit::Models::RigidBody::DataPoint dataPoint;
				dataPoint.x = srcPoints[i];
				dataPoint.xdash = dstPoints[i];
				dataSet.push_back(dataPoint);
			}

			if (dataSet.empty()) {
				throw(ofxRulr::Exception("Data set is empty!"));
			}

			this->model.initialiseParameters();

			auto fitter = make_shared<ofxNonLinearFit::Fit<ofxNonLinearFit::Models::RigidBody>>();

			double residual;
			bool success;
			auto startTime = chrono::system_clock::now();
			if (this->parameters.nlSettings.trimOutliers == 0.0f) {
				//Use full data set.
				success = fitter->optimise(this->model, &dataSet, &residual);
			}
			else {
				//Use subset with top results. This will only work if the full set is calculated previously.
				auto firstIt = dataSet.begin();
				auto lastIt = firstIt + (int)(dataSet.size() * (1.0f - this->parameters.nlSettings.trimOutliers));
				ofxNonLinearFit::Models::RigidBody::DataSet subSet(firstIt, lastIt);
				success = fitter->optimise(this->model, &subSet, &residual);
			}
			auto endTime = chrono::system_clock::now();

			sort(dataSet.begin(), dataSet.end(), [this](auto & a, auto & b) {
				return (this->model.getResidual(a) < this->model.getResidual(b));
			});

			this->result.success = success;
			this->result.totalTime = endTime - startTime;
			this->result.residual = residual;
			this->result.transform = this->model.getCachedTransform();

			this->completed = true;
		}

		//----------
		void SolveSet::solveCv() {
			

			//this->result.success = success;
			//this->result.totalTime = endTime - startTime;
			//this->result.residual = residual;
			//this->result.transform = this->model.getCachedTransform();

			//this->completed = true;
		}

		//----------
		void SolveSet::serialize(Json::Value & json) {
			{
				auto & jsonResult = json["result"];
				jsonResult["success"] << this->result.success;
				//jsonResult["totalTime"] << this->result.totalTime;
				jsonResult["residual"] << this->result.residual;
				jsonResult["transform"] << this->result.transform;
			}
			{
				auto & jsonDataSet = json["dataSet"];
				int index = 0;
				for (int i = 0; i < this->srcPoints.size(); ++i) {
					auto & jsonDataPoint = jsonDataSet[index++];
					jsonDataPoint["src"] << this->srcPoints[i];
					jsonDataPoint["dst"] << this->dstPoints[i];
				}
			}

			ofxRulr::Utils::Serializable::serialize(json["parameters"], this->parameters);
		}

		//----------
		void SolveSet::deserialize(const Json::Value & json) {
			{
				const auto & jsonResult = json["result"];
				jsonResult["success"] >> this->result.success;
				//jsonResult["totalTime"] >> this->result.totalTime;
				jsonResult["residual"] >> this->result.residual;
				jsonResult["transform"] >> this->result.transform;
			}
			{
				this->srcPoints.clear();
				this->dstPoints.clear();
				const auto & jsonDataSet = json["dataSet"];
				for (const auto & jsonDataPoint : jsonDataSet) {
					//ofxNonLinearFit::Models::RigidBody::DataPoint dataPoint;
					ofVec3f src;
					ofVec3f dst;
					jsonDataPoint["src"] >> src;
					jsonDataPoint["dst"] >> dst;
					this->srcPoints.push_back(src);
					this->dstPoints.push_back(dst);
				}
			}

			ofxRulr::Utils::Serializable::deserialize(json["parameters"], this->parameters);
		}

		//----------
		bool SolveSet::didComplete() const {
			return this->completed;
		}

		//----------
		const SolveSet::Result & SolveSet::getResult() const {
			return this->result;
		}
	}
}
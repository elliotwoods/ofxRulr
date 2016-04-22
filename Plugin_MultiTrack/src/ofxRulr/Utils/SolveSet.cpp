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
			this->dataSet.clear();
			this->fitter.reset();
		}

		//----------
		void SolveSet::setup(const vector<ofVec3f> & srcPoints, const vector<ofVec3f> & dstPoints) {
			clear();

			if (srcPoints.size() != dstPoints.size()) {
				ofLogError("SolveSet") << "Source and destination sets mismatch!";
				return;
			}

			//Copy the data.
			for (size_t i = 0; i < srcPoints.size(); ++i) {
				ofxNonLinearFit::Models::RigidBody::DataPoint dataPoint;
				dataPoint.x = srcPoints[i];
				dataPoint.xdash = dstPoints[i];
				this->dataSet.push_back(dataPoint);
			}
		}

		//----------
		void SolveSet::trySolve() {
			if (this->dataSet.empty()) {
				throw(ofxRulr::Exception("Data set is empty!"));
			}

			this->fitter = make_shared<ofxNonLinearFit::Fit<ofxNonLinearFit::Models::RigidBody>>();

			this->model.initialiseParameters();

			double residual;
			bool success;
			auto startTime = chrono::system_clock::now();
			if (this->parameters.trimOutliers == 0.0f) {
				//Use full data set.
				success = fitter->optimise(this->model, &this->dataSet, &residual);
			}
			else {
				//Use subset with top results. This will only work if the full set is calculated previously.
				auto firstIt = this->dataSet.begin();
				auto lastIt = firstIt + (int)(this->dataSet.size() * (1.0f - this->parameters.trimOutliers));
				ofxNonLinearFit::Models::RigidBody::DataSet subSet(firstIt, lastIt);
				success = fitter->optimise(this->model, &subSet, &residual);
			}
			auto endTime = chrono::system_clock::now();

			sort(this->dataSet.begin(), this->dataSet.end(), [this](auto & a, auto & b) {
				return (this->model.getResidual(a) < this->model.getResidual(b));
			});

			this->fitter.reset();

			this->result.success = success;
			this->result.totalTime = endTime - startTime;
			this->result.residual = residual;
			this->result.transform = this->model.getCachedTransform();

			this->completed = true;
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
				for (const auto & dataPoint : this->dataSet) {
					auto & jsonDataPoint = jsonDataSet[index++];
					jsonDataPoint["x"] << dataPoint.x;
					jsonDataPoint["xdash"] << dataPoint.xdash;
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
				this->dataSet.clear();
				const auto & jsonDataSet = json["dataSet"];
				for (const auto & jsonDataPoint : jsonDataSet) {
					ofxNonLinearFit::Models::RigidBody::DataPoint dataPoint;
					jsonDataPoint["x"] >> dataPoint.x;
					jsonDataPoint["xdash"] >> dataPoint.xdash;
					this->dataSet.push_back(dataPoint);
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
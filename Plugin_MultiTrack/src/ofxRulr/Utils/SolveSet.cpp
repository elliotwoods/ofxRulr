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
			this->inPoints.clear();
			this->outPoints.clear();
		}

		//----------
		void SolveSet::setup(const vector<ofVec3f> & srcPoints, const vector<ofVec3f> & dstPoints) {
			clear();

			if (srcPoints.empty()) {
				throw(ofxRulr::Exception("Data set is empty!"));
			}
			if (srcPoints.size() != dstPoints.size()) {
				throw(ofxRulr::Exception("Source and destination sets mismatch!"));
			}

			const auto dataSize = srcPoints.size();

			//Copy the data for NL optimizer.
			{
				this->dataSet.resize(dataSize);
				for (size_t i = 0; i < dataSize; ++i) {
					dataSet[i].x = srcPoints[i];
					dataSet[i].xdash = dstPoints[i];
				}
			}

			//Copy the data for CV.
			{
				inPoints.resize(dataSize);
				outPoints.resize(dataSize);
				for (size_t i = 0; i < dataSize; ++i) {
					inPoints[i] = cv::Point3f(srcPoints[i].x, srcPoints[i].y, srcPoints[i].z);
					outPoints[i] = cv::Point3f(dstPoints[i].x, dstPoints[i].y, dstPoints[i].z);
				}
			}
		}

		//----------
		void SolveSet::solveNL() {
			if (this->dataSet.empty()) {
				throw(ofxRulr::Exception("Data set is empty!"));
			}

			this->model.initialiseParameters();

			auto fitter = make_shared<ofxNonLinearFit::Fit<ofxNonLinearFit::Models::RigidBody>>();

			double residual;
			bool success;
			auto startTime = chrono::system_clock::now();
			if (this->parameters.nlSettings.trimOutliers == 0.0f) {
				//Use full data set.
				success = fitter->optimise(this->model, &this->dataSet, &residual);
			}
			else {
				//Use subset with top results. This will only work if the full set is calculated previously.
				auto firstIt = dataSet.begin();
				auto lastIt = firstIt + (int)(this->dataSet.size() * (1.0f - this->parameters.nlSettings.trimOutliers));
				ofxNonLinearFit::Models::RigidBody::DataSet subSet(firstIt, lastIt);
				success = fitter->optimise(this->model, &subSet, &residual);
			}
			auto endTime = chrono::system_clock::now();

			sort(this->dataSet.begin(), this->dataSet.end(), [this](auto & a, auto & b) {
				return (this->model.getResidual(a) < this->model.getResidual(b));
			});

			this->result.success = success;
			this->result.totalTime = endTime - startTime;
			this->result.residual = residual;
			this->result.transform = this->model.getCachedTransform();

			cout << this->result.transform << endl;

			this->completed = true;
		}

		//----------
		void SolveSet::solveCv() {
			if (this->inPoints.empty()) {
				throw(ofxRulr::Exception("Data set is empty!"));
			}

			//Estimate the transform.
			vector<uchar> inliers;
			cv::Mat transform(3, 4, CV_64F);

			auto startTime = chrono::system_clock::now();
			int ret = cv::estimateAffine3D(this->inPoints, this->outPoints, transform, inliers);
			std::cout << transform << std::endl;
			auto endTime = chrono::system_clock::now();

			this->result.success = ret != 0;
			this->result.totalTime = endTime - startTime;
			//this->result.residual = residual;
			//this->result.transform = ofMatrix4x4(transform

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
				for (const auto & dataPoint : this->dataSet) {
					auto & jsonDataPoint = jsonDataSet[index++];
					jsonDataPoint["src"] << dataPoint.x;
					jsonDataPoint["dst"] << dataPoint.xdash;
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
				this->inPoints.clear();
				this->outPoints.clear();
				const auto & jsonDataSet = json["dataSet"];
				for (const auto & jsonDataPoint : jsonDataSet) {
					ofxNonLinearFit::Models::RigidBody::DataPoint dataPoint;
					jsonDataPoint["x"] >> dataPoint.x;
					jsonDataPoint["xdash"] >> dataPoint.xdash;
					this->dataSet.push_back(dataPoint);

					cv::Point3f inPoint = cv::Point3f(dataPoint.x.x, dataPoint.x.y, dataPoint.x.z);
					cv::Point3f outPoint = cv::Point3f(dataPoint.xdash.x, dataPoint.xdash.y, dataPoint.xdash.z);
					this->inPoints.push_back(inPoint);
					this->outPoints.push_back(outPoint);
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
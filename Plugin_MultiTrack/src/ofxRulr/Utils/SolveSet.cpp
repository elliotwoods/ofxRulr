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
			auto startTime = chrono::system_clock::now();
			bool success = fitter->optimise(this->model, &this->dataSet, &residual);
			auto endTime = chrono::system_clock::now();

			this->fitter.reset();

			this->result.success = success;
			this->result.totalTime = endTime - startTime;
			this->result.residual = residual;
			this->result.transform = this->model.getCachedTransform();

			this->completed = true;
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
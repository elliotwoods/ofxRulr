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
			if (this->running) {
				this->thread.join();
			}
			this->running = false;
			this->completed = false;
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
				ofLogError("SolveSet") << "Data set is empty!";
				return;
			}

			this->running = true;
			this->startTime = chrono::system_clock::now();

			//Run the solver in a separate thread.
			auto solveBlock = [=]() {
				this->fitter = make_shared<ofxNonLinearFit::Fit<ofxNonLinearFit::Models::RigidBody>>();

				this->model.initialiseParameters();
				
				double residual;
				bool success = fitter->optimise(this->model, &this->dataSet, &residual);
				auto endTime = chrono::system_clock::now();

				this->fitter.reset();

				this->mutex.lock();
				{
					this->result.success = success;
					this->result.totalTime = endTime - startTime;
					this->result.residual = residual;
					this->result.transform = this->model.getCachedTransform();
				}
				this->mutex.unlock();

				this->running = false;
				this->completed = true;
			};

			this->thread = std::thread(solveBlock);
		}

		//----------
		bool SolveSet::isRunning() const {
			return this->running;
		}

		//----------
		bool SolveSet::didComplete() const {
			return this->completed;
		}

		//----------
		chrono::system_clock::duration SolveSet::getRunningTime() const {
			return chrono::system_clock::now() - this->startTime;
		}
		
		//----------
		SolveSet::Result SolveSet::getResult() {
			this->mutex.lock();
			{
				return this->result;
			}
			this->mutex.unlock();
		}
	}
}
#pragma once

#include "ofxNonLinearFit.h"

namespace ofxRulr {
	namespace Utils {
		class SolveSet {
		public:
			struct Result {
				Result() : success(false) {};

				ofMatrix4x4 transform;
				float residual;
				chrono::system_clock::duration totalTime;
				bool success;
			};

			SolveSet();
			~SolveSet();

			void clear();
			void setup(const vector<ofVec3f> & srcPoints, const vector<ofVec3f> & dstPoints);
			void trySolve();

			bool isRunning() const;
			bool didComplete() const;
			chrono::system_clock::duration getRunningTime() const;

			Result getResult();

		protected:
			ofxNonLinearFit::Models::RigidBody::DataSet dataSet;
			ofxNonLinearFit::Models::RigidBody model;
			shared_ptr<ofxNonLinearFit::Fit<ofxNonLinearFit::Models::RigidBody>> fitter;
			
			std::thread thread;
			std::mutex mutex;

			bool running;
			bool completed;
			chrono::system_clock::time_point startTime;
			Result result;
		};
	}
}
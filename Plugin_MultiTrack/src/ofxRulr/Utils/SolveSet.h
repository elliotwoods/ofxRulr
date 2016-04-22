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

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			bool didComplete() const;
			const Result & getResult() const;

			struct : ofParameterGroup {
				ofParameter<float> trimOutliers{ "Trim Outliers [%]", 0, 0.0f, 1.0f };

				PARAM_DECLARE("Options", trimOutliers);
			} parameters;

		protected:
			ofxNonLinearFit::Models::RigidBody::DataSet dataSet;
			ofxNonLinearFit::Models::RigidBody model;
			shared_ptr<ofxNonLinearFit::Fit<ofxNonLinearFit::Models::RigidBody>> fitter;

			bool completed;
			Result result;
		};
	}
}
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
			
			void solveNLOpt();
			void solveCv();

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			bool didComplete() const;
			const Result & getResult() const;

			struct : ofParameterGroup {
				struct : ofParameterGroup {
					ofParameter<float> trimOutliers{ "Trim Outliers [%]", 0, 0.0f, 1.0f };
					PARAM_DECLARE("NLopt", trimOutliers);
				} nlSettings;
				struct : ofParameterGroup {
					ofParameter<float> trimOutliers{ "Trim Outliers [%]", 0, 0.0f, 1.0f };
					PARAM_DECLARE("NLopt", trimOutliers);
				} cvSettings;
				PARAM_DECLARE("Settings", nlSettings);
			} parameters;

		protected:
			vector<ofVec3f> srcPoints;
			vector<ofVec3f> dstPoints;

			ofxNonLinearFit::Models::RigidBody model;

			bool completed;
			Result result;
		};
	}
}
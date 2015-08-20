#pragma once

#include <vector>
#include "ofVec2f.h"

using namespace std;

namespace ofxRulr {
	namespace Utils {
		namespace PolyFit {
			struct Model {
				vector<float> parameters;
			};

			Model fit(const vector<ofVec2f> &, int order);
			float evaluate(const Model &, float x);
		}
	}
}
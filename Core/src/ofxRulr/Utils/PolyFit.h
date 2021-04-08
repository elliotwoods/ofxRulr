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

			Model fit(const vector<glm::vec2> &, int order);
			float evaluate(const Model &, float x);
		}
	}
}
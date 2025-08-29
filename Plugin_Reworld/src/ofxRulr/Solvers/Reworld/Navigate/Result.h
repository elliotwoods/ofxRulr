#pragma once

#include "ofxRulr/Models/Reworld/Module.h"

namespace ofxRulr {
	namespace Solvers {
		namespace Reworld {
			namespace Navigate {
				struct Solution {
					Models::Reworld::AxisAngles<float> axisAngles;
				};
				typedef ofxCeres::Result<Solution> Result;
			}

		}
	}
}
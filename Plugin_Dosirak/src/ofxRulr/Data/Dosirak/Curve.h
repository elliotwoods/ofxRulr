#pragma once

#include "ofMain.h"

namespace ofxRulr {
	namespace Data {
		namespace Dosirak {
			struct Curve {
				ofFloatColor color;
				vector<glm::vec3> points;
				bool closed;

				void updatePreview();
				void drawPreview() const;
			protected:
				ofPolyline preview;
			};

			typedef map<string, Data::Dosirak::Curve> Curves;
		}
	}
}
#include "pch_Plugin_Reworld.h"
#include "AxisAngles.h"

namespace ofxRulr {
	namespace Models {
		namespace Reworld {
			//----------
			void
				drawAxisAngles(const AxisAngles<float>& axisAngles, const ofRectangle& bounds)
			{
				float radius = bounds.width > bounds.height
					? bounds.height / 2.0f
					: bounds.width / 2.0f;
				auto center = bounds.getCenter();

				auto positionValue = axisAnglesToVector(axisAngles);
				auto lineEnd = center + radius * positionValue;
				ofPushStyle();
				{
					ofNoFill();
					ofDrawCircle(center, radius);
					ofDrawLine(center, lineEnd);

					ofFill();
					ofDrawCircle(lineEnd, 2);
				}
				ofPopStyle();

			}
		}
	}
}

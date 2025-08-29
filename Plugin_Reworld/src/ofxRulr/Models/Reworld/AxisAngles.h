#pragma once

#include <glm/glm.hpp>
#include "ofMathConstants.h"

namespace ofxRulr {
	namespace Models {
		namespace Reworld {
			template<typename T>
			struct AxisAngles {
				T A;
				T B;
			};

			template<typename T>
			AxisAngles<T>
				findClosestCycleValue(const AxisAngles<T>& current, const AxisAngles<T>& target)
			{
				// Note in general we shouldn't be doing this in Rulr
				AxisAngles<T> adjusted;
				adjusted.A = target.A + std::round(current.A - target.A);
				adjusted.B = target.B + std::round(current.B - target.B);

				return adjusted;
			}

			template<typename T>
			glm::tvec2<T>
				axisAnglesToPolar(const AxisAngles<T>& axisAngles)
			{
				T a = axisAngles.A;
				T b = axisAngles.B;

				// Normalize cyclic values into [0, 1)
				a = glm::fract(a);
				b = glm::fract(b);

				// Your mapping
				T r = T(2) * a - T(2) * b + T(1);
				T thetaNorm = (a + b - T(1)) / T(2);

				// Fold r to [0,1] while compensating theta by 180° when r would be negative
				if (r > T(1)) {
					r = T(2) - r;               // 1 - (r - 1)
				}
				if (r < T(0)) {
					thetaNorm += T(0.5);        // rotate by half-turn
					r = -r;
				}

				// Two-pi in a templated-safe way
				T theta = (thetaNorm + T(0.5)) * (T) TWO_PI;

				return { r, theta };
			}

			template<typename T>
			glm::tvec2<T>
				polarToVector(const glm::tvec2<T>& polar)
			{
				const auto& r = polar[0];
				const auto& theta = polar[1];

				return {
					r * cos(theta)
					, r * sin(theta)
				};
			}

			template<typename T>
			glm::tvec2<T>
				vectorToPolar(const glm::tvec2<T>& vector)
			{
				auto r = glm::length(vector);
				auto theta = atan2(vector.y, vector.x);

				return {
					r
					, theta
				};
			}

			template<typename T>
			AxisAngles<T>
				polarToAxisAngles(const glm::tvec2<T>& polar)
			{
				const auto& r = polar[0];
				const auto& theta = polar[1];

				// axes norm coordinates are offset by half rotation from polar
				// (for axes, left = 0; for polar, right = 0)
				const auto thetaNorm = theta / (T) TWO_PI - (T) 0.5;

				// our special sauce for our lenses
				return {
					thetaNorm - (1 - r) * (T) 0.25 + (T) 0.5
					, thetaNorm + (1 - r) * (T) 0.25 + (T) 0.5
				};
			}

			template<typename T>
			glm::tvec2<T>
				axisAnglesToVector(const AxisAngles<T>& axisAngles)
			{
				auto polar = axisAnglesToPolar(axisAngles);
				return polarToVector(polar);
			}

			template<typename T>
			AxisAngles<T>
				vectorToAxisAngles(const glm::tvec2<T>& vector)
			{
				auto polar = vectorToPolar(vector);
				return polarToAxisAngles(polar);
			}

			void drawAxisAngles(const AxisAngles<float>&, const ofRectangle& bounds);
		}
	}
}
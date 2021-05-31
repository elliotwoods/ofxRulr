#include "pch_RulrCore.h"
#include "Utils.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		string Utils::formatDuration(chrono::duration<double, ratio<1, 1>> duration
		, bool showHours
		, bool showMinutes
		, bool showSeconds
		, bool showMilliseconds) {
			stringstream output;

			if (showHours) {
				auto hours = chrono::duration_cast<chrono::hours>(duration);
				duration -= hours;
				if (hours.count() > 0) {
					output << hours.count() << "h ";
				}
			}

			if (showMinutes) {
				auto minutes = chrono::duration_cast<chrono::minutes>(duration);
				duration -= minutes;
				if (minutes.count() > 0) {
					output << minutes.count() << "m ";
				}
			}

			if (showSeconds) {
				auto seconds = chrono::duration_cast<chrono::seconds>(duration);
				duration -= seconds;
				if (seconds.count() > 0) {
					output << seconds.count() << "s ";
				}
			}

			if (showMilliseconds) {
				auto milliseconds = chrono::duration_cast<chrono::milliseconds>(duration);
				duration -= milliseconds;
				if (milliseconds.count() > 0) {
					output << milliseconds.count() << "ms";
				}
			}

			return output.str();
		}

		//----------
		glm::vec3 applyTransform(const glm::mat4& matrix, const glm::vec3& vec3)
		{
			glm::vec4 vec4{ vec3.x, vec3.y, vec3.z, 1.0f };
			auto result = matrix * vec4;
			result /= result.w;
			return {
				result.x
				, result.y
				, result.z
			};
		}

		//----------
		glm::vec2 applyTransform(const glm::mat4& matrix, const glm::vec2& vec2)
		{
			glm::vec4 vec4{ vec2.x, vec2.y, 0.0f, 1.0f };
			auto result = matrix * vec4;
			result /= result.w;
			return {
				result.x
				, result.y
			};
		}

		//----------
		void speakCount(size_t count)
		{
			if (count == 0) {
				ofxAssets::sound("ofxRulr::failure").play();
			}
			else if (count <= 20) {
				auto& sound = ofxAssets::sound("ofxRulr::" + ofToString(count));
				sound.play();
			}
			else {
				ofxAssets::sound("ofxRulr::success").play();
			}
		}
	}
}

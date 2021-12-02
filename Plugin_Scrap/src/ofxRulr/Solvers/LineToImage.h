#pragma once

#include "ofxCeres.h"

namespace ofxRulr {
	namespace Solvers {
		class LineToImage : ofxCeres::Models::Base
		{
		public:
			// Line as defined by s, t
			template<typename T>
			struct Line_ {
				glm::tvec2<T> s;
				glm::tvec2<T> t;

				Line_() {

				}

				Line_(const cv::Vec4f& openCVLine) {
					this->t.x = openCVLine[0];
					this->t.y = openCVLine[1];
					this->s.x = openCVLine[2];
					this->s.y = openCVLine[3];
				}

				Line_(const glm::tvec2<T>& point, const T& angle) {
					this->s = point;
					this->t[0] = cos(angle);
					this->t[1] = sin(angle);
				}

				// ax + by + c = 0
				// NOTE : We presume the line doesn't go vertical here!
				glm::tvec3<T> getABC() const {
					// (1) a * sx + b * sy + c = 0
					// (2) a * (sx + tx) + b * (sy + ty) + c = 0
					// 
					// (3) a * tx + b * ty = 0
					// Let's say b = -1
					// (4) a * tx - ty = 0
					//     a = ty / tx
					// (5) ty/tx * sx - sy + c = 0
					//     c = sy - ty/tx * sx
					return glm::tvec3<T> {
						this->t.y / this->t.x
							, -1
							, this->s.y - this->t.y / this->t.x * this->s.x
					};
				}

				// From https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
				// From https://brilliant.org/wiki/dot-product-distance-between-point-and-a-line/?quiz=dot-product-distance-between-point-and-a-line#_=_
				T distanceToPoint(const glm::tvec2<T>& point) const {
					auto abc = this->getABC();
					return ofxCeres::VectorMath::distanceLineToPoint(abc, point);
				}

				template<typename T2>
				void fromParameters(T2* const parameters) {
					this->s[0] = (T) parameters[0];
					this->s[1] = (T) parameters[1];
					this->t[0] = (T)sin(parameters[2]);
					this->t[1] = (T)cos(parameters[2]);
				}

				template<typename T2>
				void toParameters(T2* parameters) const {
					parameters[0] = (T2) this->s[0];
					parameters[1] = (T2) this->s[1];
					parameters[2] = (T2)atan2(this->t[1], this->t[2]);
				}

				void drawOnImage(cv::Mat & image) const {
					auto start = cv::Point2f(this->s.x, this->s.y);
					auto t = cv::Point2f(this->t.x, this->t.y);
					cv::line(image, start - 10000 * t, start + 10000 * t, cv::Scalar(255));
				}

				void serialize(nlohmann::json& json) const {
					Utils::serialize(json, "s", this->s);
					Utils::serialize(json, "t", this->t);
				}

				void deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "s", this->s);
					Utils::deserialize(json, "t", this->t);
				}

				float meanMaskedValue(const cv::Mat& image) const {
					cv::Mat lineImage = cv::Mat::zeros(image.size(), image.type());
					this->drawOnImage(lineImage);

					// Count non zero pixels in the line
					auto nonZeroPixelCount = cv::countNonZero(lineImage);

					// Take the sum of the normalized difference image masked by the line
					cv::Mat maskedPixels;
					cv::bitwise_and(image, lineImage, maskedPixels);
					auto sumOfMaskedValues = cv::sum(maskedPixels)[0];
					auto meanMaskedValue = sumOfMaskedValues / (double)nonZeroPixelCount;

					return meanMaskedValue;
				}
			};

			typedef Line_<float> Line;

			struct Solution {
				Line line;
				float meanResidual;
			};

			static ofxCeres::SolverSettings defaultSolverSettings();

			typedef ofxCeres::Result<Solution> Result;

			static Result solve(const cv::Mat& image
				, uint8_t minimumValue
				, const ofxCeres::SolverSettings& solverSettings = defaultSolverSettings());

			static float getResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution&);

			static float getMeanResidual(const vector<glm::vec2>& points
				, const vector<float>& weights
				, const Solution&);

			static void imageToData(const cv::Mat& image
				, uint8_t minimumValue
				, vector<glm::vec2> & points
				, vector<float> & weights);
		};
	}
}
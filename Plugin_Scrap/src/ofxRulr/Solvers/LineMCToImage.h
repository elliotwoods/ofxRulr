#pragma once

#include "ofxCeres.h"

namespace ofxRulr {
	namespace Solvers {
		/// <summary>
		/// MC refers to the line definition, i.e. y = mx + c
		/// </summary>
		class LineMCToImage : ofxCeres::Models::Base
		{
		public:
			template<typename T>
			struct Line_ {
				T m;
				T c;

				Line_() {

				}

				Line_(const cv::Vec4f& openCVLine) {
					this->m = openCVLine[1] / openCVLine[0];
					this->c = openCVLine[3] - openCVLine[2] * this->m;
				}

				template<typename T2>
				void fromParameters(T2* const parameters) {
					this->m = parameters[0];
					this->c = parameters[1];
				}

				template<typename T2>
				void toParameters(T2* parameters) const {
					parameters[0] = this->m;
					parameters[1] = this->c;
				}

				void drawOnImage(cv::Mat & image) {
					auto start = cv::Point2f(0, this->c);
					auto end = cv::Point2f(image.cols, image.cols * this->m + this->c);
					cv::line(image, start, end, cv::Scalar(255));
				}

				void serialize(nlohmann::json& json) {
					Utils::serialize(json, "m", this->m);
					Utils::serialize(json, "c", this->c);
				}

				void deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "m", this->m);
					Utils::deserialize(json, "c", this->c);
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
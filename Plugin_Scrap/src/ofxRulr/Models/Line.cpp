#include "pch_Plugin_Scrap.h"
#include "Line.h"

namespace ofxRulr {
	namespace Models {
		//----------
		void
			Line::drawOnImage(cv::Mat& image) const
		{
			auto whereMeetsImageEdges = this->getImageEdgeIntersects(image.size());
			if (whereMeetsImageEdges.size() >= 2) {
				const auto start = whereMeetsImageEdges.begin();
				cosnt auto end = whereMeetsImageEdges.end();
			}
			cv::line(image
				, *start
				, *end
				, cv::Scalar(255));
		}

		//----------
		void 
			Line::serialize(nlohmann::json& json) const
		{
			Utils::serialize(json, "s", this->s);
			Utils::serialize(json, "t", this->t);
		}

		//----------
		void
			Line::deserialize(const nlohmann::json& json)
		{
			Utils::deserialize(json, "s", this->s);
			Utils::deserialize(json, "t", this->t);
		}

		//----------
		float
			Line::meanMaskedValue(const cv::Mat& image) const
		{
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

		//----------
		map<Line::ImageEdge, glm::vec2>
			Line::getImageEdgeIntersects(const cv::Size& imageSize) const
		{
			map<ImageEdge, glm::vec2> results;

			// Top
			{
				auto x = this->s.x - this->s.y * this->t.x / this->t.y;
				if (x > 0 && x < imageSize.width) {
					results[ImageEdge::Top] = { x, 0 };
				}
			}

			// Bottom
			{
				auto x = this->s.x + (imageSize.height - this->s.y) * this->t.x / this->t.y;
				if (x > 0 && x < imageSize.width) {
					results[ImageEdge::Bottom] = { x, imageSize.height };
				}
			}

			// Left
			{
				auto y = this->s.y - this->s.x * this->t.y / this->t.x;
				if (y > 0 && y < imageSize.height) {
					results[ImageEdge::Left] = { 0, y };
				}
			}

			// Right
			{
				auto y = this->s.y + (imageSize.width - this->s.x) * this->t.y / this->t.x;
				if (y > 0 && y < imageSize.height) {
					results[ImageEdge::Right] = { imageSize.width, y };
				}
			}

			return results;
		}
	}
}
#pragma once

namespace ofxRulr {
	namespace Models {
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

			Line_(const glm::tvec2<T>& s, const glm::tvec2<T>& t)
			: s(s)
			, t(t) {
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

			glm::tvec2<T> getClosestPointTo(const glm::tvec2<T>& a) const {
				auto u = ofxCeres::VectorMath::dot(a - this->s, this->t);
				return this->s + u * this->t;
			}

			template<typename T2>
			void fromParameters(T2* const parameters) {
				this->s[0] = (T)parameters[0];
				this->s[1] = (T)parameters[1];
				this->t[0] = (T)sin(parameters[2]);
				this->t[1] = (T)cos(parameters[2]);
			}

			template<typename T2>
			void toParameters(T2* parameters) const {
				parameters[0] = (T2)this->s[0];
				parameters[1] = (T2)this->s[1];
				parameters[2] = (T2)atan2(this->t[1], this->t[2]);
			}

			template<typename T2>
			Line_<T2> castTo() const {
				return Line_<T2>((glm::tvec2<T2>) this->s, (glm::tvec2<T2>) this->t);
			}
		};

		class Line : public Line_<float> {
		public:
			Line()
				: Line_<float>() {}

			Line(const cv::Vec4f& openCVLine)
			: Line_<float>(openCVLine) { }

			Line(const glm::vec2& point, const float& angle)
				: Line_<float>(point, angle) { }

			void drawOnImage(cv::Mat& image) const;

			void serialize(nlohmann::json& json) const;
			void deserialize(const nlohmann::json& json);

			float meanMaskedValue(const cv::Mat& image) const;

			enum class ImageEdge {
				Top
				, Left
				, Right
				, Bottom
			};

			map<ImageEdge, glm::vec2> getImageEdgeIntersects(const cv::Size&) const;
		};
	}
}
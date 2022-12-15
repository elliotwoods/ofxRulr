#pragma once

#include "DistortedGrid.h"
#include "SurfacePosition.h"

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct IntegratedSurface_ {
			DistortedGrid_<T, SurfacePosition_<T>> distortedGrid;

			template<typename T2>
			IntegratedSurface_<T2> castTo() const
			{
				IntegratedSurface_<T2> newIntegratedSurface;
				newIntegratedSurface.distortedGrid = this->distortedGrid.castTo<T2, SurfacePosition_<T2>>();
				return newIntegratedSurface;
			}

			// Remarkable 2022-12 page 5
			static T getHeightAt2(const T& x1
				, const T& x2
				, const T& dz_dx_1
				, const T& dz_dx_2
				, const T& height1)
			{
				const auto A = (dz_dx_1 - dz_dx_2) / ((T) 2 * (x1 - x2));
				const auto B = dz_dx_1 - (T) 2 * A * x1;
				const auto C = height1 - (A * x1 * x1 + B * x1);

				return A * x2 * x2 + B * x2 + C;
			}

			size_t getResidualCount() const
			{
				size_t count = 0;
				for (size_t j = 1; j < this->distortedGrid.positions.size(); j++) {
					const auto& row = this->distortedGrid.positions[j];
					for (size_t i = 1; i < row.size(); i++) {
						count++;
					}
				}
				return count;
			}

			void integrateHeights(T* residuals = nullptr, glm::tvec3<T>* residualPositions = nullptr)
			{
				if (this->distortedGrid.positions.empty()) {
					throw(ofxCeres::Exception("Grid is empty"));
				}

				// Similar to above but store the height

				// First go along bottom row
				//{
				//	auto& row = this->distortedGrid.positions.front();
				//	for (size_t i = 1; i < row.size(); i++) {
				//		auto& position = row[i];
				//		const auto& leftPosition = row[i - 1];

				//		const auto heightFromX = getHeightAt2(leftPosition.currentPosition.x
				//			, position.currentPosition.x
				//			, leftPosition.dz_dx()
				//			, position.dz_dx()
				//			, leftPosition.getHeight());

				//		position.setHeight(heightFromX);
				//	}
				//}

				// Then go along left edge
				{
					for (size_t j = 1; j < this->distortedGrid.positions.size(); j++) {
						const auto& downPosition = this->distortedGrid.at(0, j - 1);
						auto& position = this->distortedGrid.at(0, j);

						const auto heightFromY = getHeightAt2(downPosition.currentPosition.y
							, position.currentPosition.y
							, downPosition.dz_dy()
							, position.dz_dy()
							, downPosition.getHeight());

						position.setHeight(heightFromY);
					}
				}

				// Then do other rows
				for (size_t j = 1; j < this->distortedGrid.positions.size(); j++) {
					auto& row = this->distortedGrid.positions[j];
					for (size_t i = 1; i < row.size(); i++) {
						auto& position = this->distortedGrid.at(i, j);
						const auto& leftPosition = this->distortedGrid.at(i - 1, j);
						const auto& downPosition = this->distortedGrid.at(i, j - 1);

						// We calculate the height (z) of the current vertex given 2 neighboring vertices

						const auto heightFromX = getHeightAt2(leftPosition.currentPosition.x
							, position.currentPosition.x
							, leftPosition.dz_dx()
							, position.dz_dx()
							, leftPosition.getHeight());

						const auto heightFromY = getHeightAt2(downPosition.currentPosition.y
							, position.currentPosition.y
							, downPosition.dz_dy()
							, position.dz_dy()
							, downPosition.getHeight());

						if (residuals) {
							*residuals++ = heightFromY - heightFromX;
						}

						position.setHeight(heightFromY);

						if (residualPositions) {
							*residualPositions++ = position.currentPosition;
						}
					}
				}
			}
		};

		typedef IntegratedSurface_<float> IntegratedSurface;
	}
}
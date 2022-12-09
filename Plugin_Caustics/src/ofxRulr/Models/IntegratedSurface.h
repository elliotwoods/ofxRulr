#pragma once

#include "DistortedGrid.h"

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct SurfacePosition_ : DistortedGridPosition_<T>
		{
			glm::tvec3<T> normal;
			glm::tvec3<T> target;

			T dz_dx() const
			{
				return tan(atan2(normal.z, normal.x) - acos(0));
			}

			T dz_dy() const
			{
				return tan(atan2(normal.z, normal.y) - acos(0));
			}

			T getHeight() const
			{
				return this->currentPosition.z;
			}

			void setHeight(T height)
			{
				this->currentPosition.z = height;
			}

			template<typename T2>
			SurfacePosition_<T2> castTo() const
			{
				SurfacePosition_<T2> newPosition;

				newPosition.normal = (glm::tvec3<T2>) this->normal;
				newPosition.target = (glm::tvec3<T2>) this->target;

				newPosition.initialPosition = (glm::tvec3<T2>) this->initialPosition;
				newPosition.rightVector = (glm::tvec3<T2>) this->rightVector;
				newPosition.downVector = (glm::tvec3<T2>) this->downVector;
				newPosition.currentPosition = (glm::tvec3<T2>) this->currentPosition;

				return newPosition;
			}
		};

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

			void getResiduals(T* residuals) const
			{
				// Calculate the 2 normal routes for each vertex and the disparity between them
				// As per diagram at bottom of page 5 of reMarkable 2022-12
				// Note that in the maths we use 'y' but here we use 'z' for height

				for (size_t j = 1; j < this->distortedGrid.positions.size(); j++) {
					const auto& row = this->distortedGrid.positions[j];
					for (size_t i = 1; i < row.size(); i++) {
						const auto& position = this->distortedGrid.at(i, j);
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
							, leftPosition.getHeight());

						// The residual is the difference between the 2
						*residuals++ = heightFromY - heightFromX;
					}
				}
			}

			void integrateHeights()
			{
				// Similar to above but store the height

				// First go along bottom row
				{
					auto& row = this->distortedGrid.positions.front();
					for (size_t i = 1; i < row.size(); i++) {
						auto& position = row[i];
						const auto& leftPosition = row[i - 1];

						const auto heightFromX = getHeightAt2(leftPosition.currentPosition.x
							, position.currentPosition.x
							, leftPosition.dz_dx()
							, position.dz_dx()
							, leftPosition.getHeight());

						position.setHeight(heightFromX);
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
							, leftPosition.getHeight());

						position.setHeight((heightFromX + heightFromY) / (T)2);
					}
				}
			}
		};

		typedef IntegratedSurface_<float> IntegratedSurface;
	}
}
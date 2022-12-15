#pragma once

#include "DistortedGrid.h"
#include "SurfacePosition.h"

namespace ofxRulr {
	namespace Models {
		namespace VM = ofxCeres::VectorMath;

		struct SurfaceSectionSettings {
			size_t i_start;
			size_t j_start;
			size_t width;
			size_t height;

			size_t i_end() const {
				return this->i_start + this->width;
			}

			size_t j_end() const {
				return this->j_start + this->height;
			}

			vector<double*> initParameters(const DistortedGrid_<double, SurfacePosition_<double>> &grid) const
			{
				vector<double*> parameters;
				for (size_t j = this->j_start; j < this->j_end(); j++) {
					const auto rowData = new double[this->width];
					auto rowDataMover = rowData;
					for (size_t i = this->i_start; i < this->i_end(); i++) {
						*rowDataMover++ = grid.at(i, j).currentPosition.z;
					}
					parameters.push_back(rowData);
				}
				return parameters;
			}

			void applyParameters(DistortedGrid_<double, SurfacePosition_<double>>& grid
				, vector<double*> heightParameters) const
			{
				auto rowIterator = heightParameters.begin();
				for (size_t j = this->j_start; j < this->j_end(); j++) {
					auto rowDataMover = *rowIterator++;
					for (size_t i = this->i_start; i < this->i_end(); i++) {
						grid.at(i, j).currentPosition.z = *rowDataMover++;
					}
				}
			}
		};

		template<typename T>
		struct HeightSectionParameters_ {
			const T* const* heights;
			SurfaceSectionSettings sectionSettings;

			HeightSectionParameters_(const SurfaceSectionSettings& sectionSettings
				, const T* const* heights)
				: sectionSettings(sectionSettings)
				, heights(heights)
			{
			}

			bool includes(size_t i, size_t j) const {
				return i >= this->sectionSettings.i_start
					&& j >= this->sectionSettings.j_start
					&& i < this->sectionSettings.i_start + this->sectionSettings.width
					&& j < this->sectionSettings.j_start + this->sectionSettings.height;
			}

			T getHeight(size_t i, size_t j) const {
				if (i == 0 && j == 0) {
					// special case where we ignore the parameters and just have 0 for first vertex height
					return (T) 0;
				}

				if (!this->includes(i, j)) {
					throw(ofxCeres::Exception("Indices outside of height section"));
				}
				return this->heights[j - this->sectionSettings.j_start][i - this->sectionSettings.i_start];
			}
		};

		template<typename T>
		struct Surface_ {
			DistortedGrid_<T, SurfacePosition_<T>> distortedGrid;

			template<typename T2>
			Surface_<T2> castTo() const
			{
				Surface_<T2> newSurface;
				newSurface.distortedGrid = this->distortedGrid.castTo<T2, SurfacePosition_<T2>>();
				return newSurface;
			}

			size_t getParameterCount() const
			{
				bool firstVertex = true;
				size_t count = 0;
				for (auto& row : this->distortedGrid.positions) {
					for (auto& position : row) {
						if (firstVertex) {
							firstVertex = false;
						}
						else {
							count++;
						}
					}
				}
				return count;
			}

			void fromParameters(const T* const heightMap)
			{
				bool firstVertex = true;
				auto heightMapMover = heightMap;
				for (auto& row : this->distortedGrid.positions) {
					for (auto& position : row) {
						if (firstVertex) {
							// To clamp the surface we need to fix at least one vertex
							position.currentPosition.z = (T)0;
							firstVertex = false;
						}
						else {
							position.currentPosition.z = *heightMapMover++;
						}
					}
				}
			}

			void toParameters(T* heightMap) const
			{
				const auto rows = this->distortedGrid.rows();
				const auto cols = this->distortedGrid.cols();
				for (size_t j = 0; j < rows; j++) {
					for (size_t i = 0; i < cols; i++) {
						if (i == 0 && j == 0) {
							// first vertex is always zero
							continue;
						}
						*heightMap++ = this->distortedGrid.at(i, j).currentPosition.z;
					}
				}
			}

			size_t getResidualCount() const
			{
				size_t count = 0;
				for (auto& row : this->distortedGrid.positions) {
					for (auto& position : row) {
						count += 3;
					}
				}
				return count;
			}

			void getResiduals(T* residuals) const
			{
				for (size_t j = 0; j < this->distortedGrid.rows(); j++) {
					for (size_t i = 0; i < this->distortedGrid.cols(); i++) {
						auto estimatedNormal = this->estimateNormal(i, j);
						auto normal = this->distortedGrid.at(i, j).normal;
						auto delta = estimatedNormal - normal;
						*residuals++ = delta[0];
						*residuals++ = delta[1];
						*residuals++ = delta[2];
					}
				}
			}

			glm::tvec3<T>
				estimateNormal(size_t i, size_t j) const
			{
				T totalWeight = (T)0;
				glm::tvec3<T> accumulateNormal{ 0, 0, 0 };

				const auto& center = this->distortedGrid.at(i, j);

				if (j > 0) {
					const auto& down = this->distortedGrid.at(i, j - 1);
					const auto to_down = down.currentPosition - center.currentPosition;

					if (i > 0) {
						const auto& left = this->distortedGrid.at(i - 1, j);

						const auto to_left = left.currentPosition - center.currentPosition;

						const auto normal = VM::normalize(VM::cross(to_left, to_down));

						const auto weight = acos(VM::dot(to_left, to_down));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}

					if (i < this->distortedGrid.cols() - 1) {
						const auto& right = this->distortedGrid.at(i + 1, j);
						const auto to_right = right.currentPosition - center.currentPosition;

						const auto normal = VM::normalize(VM::cross(to_down, to_right));
						const auto weight = acos(VM::dot(to_right, to_down));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}
				}
				if (j < this->distortedGrid.rows() - 1) {
					const auto& up = this->distortedGrid.at(i, j + 1);
					const auto to_up = up.currentPosition - center.currentPosition;

					if (i > 0) {
						const auto& left = this->distortedGrid.at(i - 1, j);

						const auto to_left = left.currentPosition - center.currentPosition;

						const auto normal = VM::normalize(VM::cross(to_up, to_left));

						const auto weight = acos(VM::dot(to_left, to_up));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}

					if (i < this->distortedGrid.cols() - 1) {
						const auto& right = this->distortedGrid.at(i + 1, j);
						const auto to_right = right.currentPosition - center.currentPosition;

						const auto normal = VM::normalize(VM::cross(to_right, to_up));
						const auto weight = acos(VM::dot(to_right, to_up));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}
				}

				return accumulateNormal / totalWeight;
			}

			template<typename T2>
			glm::tvec3<T2>
				estimateIndividualNormal(const T2* const heights
					, size_t i
					, size_t j
					, size_t cols) const
			{
				auto totalWeight = (T2)0;
				glm::tvec3<T2> accumulateNormal{ 0, 0, 0 };

				auto getVertex = [&](size_t i, size_t j) {
					auto vertex = (glm::tvec3<T2>) this->distortedGrid.at(i, j).initialPosition;
					if (i != 0 || j != 0) {
						vertex.z = heights[(i + j * cols) - 1];
					}
					return vertex;
				};

				const auto center = getVertex(i, j);

				if (j > 0) {
					const auto down = getVertex(i, j - 1);
					const auto to_down = down - center;

					if (i > 0) {
						const auto left = getVertex(i - 1, j);

						const auto to_left = left - center;

						const auto normal = VM::normalize(VM::cross(to_left, to_down));

						const auto weight = acos(VM::dot(to_left, to_down));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}

					if (i < this->distortedGrid.cols() - 1) {
						const auto right = getVertex(i + 1, j);
						const auto to_right = right - center;

						const auto normal = VM::normalize(VM::cross(to_down, to_right));
						const auto weight = acos(VM::dot(to_right, to_down));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}
				}
				if (j < this->distortedGrid.rows() - 1) {
					const auto up = getVertex(i, j + 1);
					const auto to_up = up - center;

					if (i > 0) {
						const auto left = getVertex(i - 1, j);

						const auto to_left = left - center;

						const auto normal = VM::normalize(VM::cross(to_up, to_left));

						const auto weight = acos(VM::dot(to_left, to_up));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}

					if (i < this->distortedGrid.cols() - 1) {
						const auto& right = getVertex(i + 1, j);
						const auto to_right = right - center;

						const auto normal = VM::normalize(VM::cross(to_right, to_up));
						const auto weight = acos(VM::dot(to_right, to_up));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}
				}

				return accumulateNormal / totalWeight;
			}

			//----------
			// This function esimtates the normal given the parameters for the heights within a specific section
			template<typename T2>
			glm::tvec3<T2>
				estimateIndividualNormalWithinSection(const HeightSectionParameters_<T2>& heightSectionParameters
					, size_t i
					, size_t j) const
			{
				auto totalWeight = (T2)0;
				glm::tvec3<T2> accumulateNormal{ 0, 0, 0 };

				auto getVertex = [&](size_t i, size_t j) {
					auto vertex = (glm::tvec3<T2>) this->distortedGrid.at(i, j).initialPosition;
					vertex.z = heightSectionParameters.getHeight(i, j);
					return vertex;
				};

				const auto center = getVertex(i, j);

				if (j > 0) {
					const auto down = getVertex(i, j - 1);
					const auto to_down = down - center;

					if (i > 0) {
						const auto left = getVertex(i - 1, j);

						const auto to_left = left - center;

						const auto normal = VM::normalize(VM::cross(to_left, to_down));

						const auto weight = acos(VM::dot(to_left, to_down));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}

					if (i < this->distortedGrid.cols() - 1) {
						const auto right = getVertex(i + 1, j);
						const auto to_right = right - center;

						const auto normal = VM::normalize(VM::cross(to_down, to_right));
						const auto weight = acos(VM::dot(to_right, to_down));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}
				}
				if (j < this->distortedGrid.rows() - 1) {
					const auto up = getVertex(i, j + 1);
					const auto to_up = up - center;

					if (i > 0) {
						const auto left = getVertex(i - 1, j);

						const auto to_left = left - center;

						const auto normal = VM::normalize(VM::cross(to_up, to_left));

						const auto weight = acos(VM::dot(to_left, to_up));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}

					if (i < this->distortedGrid.cols() - 1) {
						const auto& right = getVertex(i + 1, j);
						const auto to_right = right - center;

						const auto normal = VM::normalize(VM::cross(to_right, to_up));
						const auto weight = acos(VM::dot(to_right, to_up));

						accumulateNormal += weight * normal;
						totalWeight += weight;
					}
				}

				return accumulateNormal / totalWeight;
			}

			void serialize(nlohmann::json& json)
			{
				this->distortedGrid.serialize(json["distortedGrid"]);
			}

			void deserialize(const nlohmann::json& json)
			{
				if (json.contains("distortedGrid")) {
					this->distortedGrid.deserialize(json["distortedGrid"]);
				}
			}

			void poissonStep(float factor, bool ignoreEdges)
			{
				// We work with int because we accept negative indices (and use as mirrored)
				const auto rows = (int) this->distortedGrid.rows();
				const auto cols = (int) this->distortedGrid.cols();

				auto getPosition = [&](int i, int j) {
					// With Neumann Boundary we use negative indices also
					if (i < 0) {
						i = -i;
					}
					else if (i > cols - 1) {
						i = cols - (i - cols) - 1;
					}

					if (j < 0) {
						j = -j;
					}
					else if (j > rows - 1) {
						j = rows - (j - rows) - 1;
					}

					return this->distortedGrid.at(i, j);
				};

				auto getPositionSimple = [&](int i, int j) {
					return this->distortedGrid.at(i, j);
				};

				auto getVertex = [&](int i, int j) {
					return (glm::tvec3<T>) getPositionSimple(i, j).currentPosition;
				};

				auto getNormal = [&](int i, int j) {
					auto normal = (glm::tvec3<T>) getPosition(i, j).normal;
					normal /= normal.z; // normalize with z = 1 for this maths
					return normal;
				};

				const auto dx = getVertex(1, 0).x - getVertex(0, 0).x;
				const auto dy = getVertex(0, 1).y - getVertex(0, 0).y;

				for (int j = 0; j < rows; j++) {
					for (int i = 0; i < cols; i++) {
						if (ignoreEdges) {
							if (i == 0 || j == 0 || i == cols - 1 || j == rows - 1) {
								continue;
							}
						}

						// Don't affect first vertex
						if (i == 0 && j == 0) {
							//continue;
						}

						T localAverage = 0;

						// With Neumann Boundary we use negative indices also
						const auto center = getVertex(i, j);
						glm::tvec3<T> left, right, up, down;

						// Symmetric differential boundary (trying this hack)
						if (i == 0) {
							right = getVertex(i + 1, j);
							left = (center - right) + center;
						}
						else if (i == cols - 1) {
							left = getVertex(i - 1, j);
							right = (center - left) + center;
						}
						else {
							left = getVertex(i - 1, j);
							right = getVertex(i + 1, j);
						}

						if (j == 0) {
							down = getVertex(i, j + 1);
							up = (center - down) + center;
						}
						else if (j == rows - 1) {
							up = getVertex(i, j - 1);
							down = (center - up) + center;
						}
						else {
							up = getVertex(i, j - 1);
							down = getVertex(i, j + 1);
						}

						//auto left = getVertex(i - 1, j);
						//auto right = getVertex(i + 1, j);
						//auto up = getVertex(i, j - 1);
						//auto down = getVertex(i, j + 1);
						
						//From the optics paper
						//dot(grad, dot(grad, height)) == dot(grad, norm);

						const auto normal_center = getNormal(i, j);
						const auto normal_left = getNormal(i - 1, j);
						const auto normal_right = getNormal(i + 1, j);
						const auto normal_up = getNormal(i, j - 1);
						const auto normal_down = getNormal(i, j + 1);

						// Div of normal field (note that boundaries are mirrored so div is 0 in that component on edges
						// We use dx and dy constants here since mirroring might have occured above on the actual vertex positions
						const auto dNx_dx = (i != 0)
							? ((i != cols - 1)
								? (normal_right.x - normal_left.x) / (dx * 2)
								: (normal_center.x - normal_left.x) / dx
								)
							: (normal_right.x - normal_center.x) / dx;

						const auto dNy_dy = (j != 0)
							? ((j != rows - 1)
								? (normal_down.y - normal_up.y) / (dy * 2)
								: (normal_center.y - normal_up.y) / dy
								)
							: (normal_down.y - normal_center.y) / dy;

						auto div_norm = dNx_dx + dNy_dy;

						auto delta = up.z + down.z + left.z + right.z - 4 * center.z + div_norm;

						this->distortedGrid.at(i, j).currentPosition.z += factor * delta / 4;
					}
				}

				// Here we reposition the whole grid so that the minimum height is at 0 since we didn't have any boundary positions within the solve itself
				if (true) {
					// Find the minimum height
					T minHeight = std::numeric_limits<T>::max();
					for (int j = 0; j < rows; j++) {
						for (int i = 0; i < cols; i++) {
							const auto z = getVertex(i, j).z;
							if (z < minHeight) {
								minHeight = z;
							}
						}
					}

					// Remove the minimum height
					for (int j = 0; j < rows; j++) {
						for (int i = 0; i < cols; i++) {
							this->distortedGrid.at(i, j).currentPosition.z -= minHeight;
						}
					}
				}
			}
		};

		typedef Surface_<float> Surface;
	}
}
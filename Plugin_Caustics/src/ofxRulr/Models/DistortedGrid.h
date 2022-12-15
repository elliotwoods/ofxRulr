#pragma once

#include "ofxCeres.h"
#include <vector>

namespace ofxRulr {
	namespace Models {
		template<typename T>
		struct DistortedGridPosition_
		{
			glm::tvec3<T> initialPosition;
			glm::tvec3<T> rightVector;
			glm::tvec3<T> downVector;
			glm::tvec3<T> currentPosition;
			
			DistortedGridPosition_()
			{

			}

			void setParameters(const T* parameters)
			{
				this->currentPosition = this->initialPosition
					+ rightVector * atan(parameters[0]) / (T) ((PI / 2) * 4) // to get to -0.25...0.25
					+ downVector * atan(parameters[1]) / (T) ((PI / 2) * 4);
			}
		};

		template<typename T, typename PositionType>
		struct DistortedGrid_ {
			std::vector<std::vector<PositionType>> positions;

			template<typename T2, typename PositionType2>
			DistortedGrid_<T2, typename PositionType2> castTo() const
			{
				DistortedGrid_<T2, typename PositionType2> newDistortedGrid;

				for (const auto& row : this->positions) {
					// Add a new row
					newDistortedGrid.positions.resize(newDistortedGrid.positions.size() + 1);
					auto& newRow = newDistortedGrid.positions.back();

					for (const auto& position : row) {
						const auto newPosition = position.castTo<T2>();
						newRow.push_back(newPosition);
					}
				}

				return newDistortedGrid;
			}

			void initGrid(size_t size, T scale)
			{
				if (size < 2) {
					throw(ofxCeres::Exception("Minimum DistortedGrid size is 2x2"));
				}

				this->positions.clear();

				for (size_t j = 0; j < size; j++) {
					std::vector<PositionType> rowOfPositions;
					for (size_t i = 0; i < size; i++) {
						PositionType position;
						position.initialPosition = glm::tvec3<T>(
							scale * ((T)i / (T)(size - 1) - (T) 0.5)
							, scale * ((T)j / (T)(size - 1) - (T) 0.5)
							, (T)0
							);
						position.currentPosition = position.initialPosition;
						rowOfPositions.push_back(position);
					}
					this->positions.push_back(rowOfPositions);
				}

				this->calculateDirectionVectors();
			}

			void initFromPreviousGrid(const DistortedGrid_<T, PositionType>& previousGrid)
			{
				this->positions.clear();

				for (size_t j = 0; j < previousGrid.positions.size(); j++) {
					const auto& previousRow = previousGrid.positions[j];
					this->positions.push_back(std::vector<SurfacePosition>());
					auto& newRow = this->positions.back();

					for (size_t i = 0; i < previousRow.size() i++) {
						auto position = make_shared<PositionType>();
						position->initialPosition = previousRow[i].currentPosition;
						position->currentPosition = position.initialPosition;
						newRow.push_back(position);
					}
				}

				this->calculateDirectionVectors();
			}

			void calculateDirectionVectors()
			{
				for (size_t j = 0; j < this->positions.size(); j++) {
					auto& row = this->positions[j];
					for (size_t i = 0; i < row.size(); i++) {
						auto& position = row[i];

						const auto& thisPos = row[i].initialPosition;

						// right vector
						if (i == 0 || i == row.size() - 1) {
							position.rightVector = glm::tvec3<T>(0.0);
						}
						else {
							const auto& leftPos = row[i - 1].initialPosition;
							const auto& rightPos = row[i + 1].initialPosition;
							position.rightVector = (rightPos - leftPos) / (T)2;
						}

						// down vector
						if (j == 0 || j == this->positions.size() - 1) {
							position.downVector = glm::tvec3<T>(0.0); ;
						}
						else {
							position.downVector = (this->positions[j + 1][i].initialPosition - this->positions[j - 1][i].initialPosition) / (T)2;
						}
					}
				}
			}

			void fromParameters(const T* parameters)
			{
				auto movingParameters = parameters;

				for (size_t j = 0; j < this->positions.size(); j++) {
					auto& row = this->positions[j];
					for (size_t i = 0; i < row.size(); i++) {
						auto& position = row[i];
						position.setParameters(movingParameters);
						movingParameters += 2;
					}
				}
			}

			size_t getParameterCount() const
			{
				size_t count = 0;
				for (const auto& row : this->positions) {
					for (const auto& point : row) {
						count += 2;
					}
				}
				return count;
			}

			const PositionType &
				at(size_t i, size_t j) const
			{
				return this->positions[j][i];
			}

			PositionType&
				at(size_t i, size_t j)
			{
				return this->positions[j][i];
			}

			size_t cols() const
			{
				if (this->rows() == 0) {
					return 0;
				}
				else {
					return this->positions.front().size();
				}
			}

			size_t rows() const
			{
				return this->positions.size();
			}

			bool inside(size_t i, size_t j) const
			{
				return i >= 0 && j >= 0
					&& i < this->cols() && j < this->rows();
			}

			bool inside(int i, int j) const
			{
				return i >= 0 && j >= 0
					&& i < this->cols() && j < this->rows();
			}

			void serialize(nlohmann::json& json)
			{
				auto rows = this->rows();
				auto cols = this->cols();

				json["rows"] = rows;
				json["cols"] = cols;

				auto& jsonPositions = json["positions"];
				for (size_t j = 0; j < rows; j++) {
					nlohmann::json jsonRow;
					for (size_t i = 0; i < cols; i++) {
						nlohmann::json jsonPosition;
						const auto& position = this->at(i, j);
						position.serialize(jsonPosition);
						jsonRow.push_back(jsonPosition);
					}
					jsonPositions.push_back(jsonRow);
				}
			}

			void deserialize(const nlohmann::json& json)
			{
				if (!json.contains("rows")) {
					return;
				}
				if (!json.contains("cols")) {
					return;
				}

				auto rows = (size_t)json["rows"];
				auto cols = (size_t)json["cols"];

				if (!json.contains("positions")) {
					return;
				}

				auto& jsonPositions = json["positions"];

				this->positions.clear();
				for (size_t j = 0; j < rows; j++) {
					this->positions.push_back(vector<PositionType>());
					auto& row = this->positions.back();

					const auto& jsonRow = jsonPositions[j];
					for (size_t i = 0; i < cols; i++) {
						const auto& jsonPosition = jsonRow[i];
						PositionType position;
						position.deserialize(jsonPosition);
						row.push_back(position);
					}
				}
			}
		};
	}
}
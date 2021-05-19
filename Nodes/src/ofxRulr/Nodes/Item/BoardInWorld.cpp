#include "pch_RulrNodes.h"
#include "BoardInWorld.h"
#include "AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			BoardInWorld::BoardInWorld()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				BoardInWorld::getTypeName() const
			{
				return "Item::BoardInWorld";
			}

			//----------
			void
				BoardInWorld::init()
			{
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<AbstractBoard>();

				this->onDrawObject += [this]() {
					auto board = this->getInput<AbstractBoard>();
					if (board) {
						board->drawObject();
					}
				};
			}

			//----------
			void
				BoardInWorld::update()
			{
			}

			//----------
			bool BoardInWorld::findBoard(cv::Mat image
				, vector<cv::Point2f>& imagePoints
				, vector<cv::Point3f>& objectPoints
				, vector<cv::Point3f>& worldPoints
				, FindBoardMode findBoardMode
				, cv::Mat cameraMatrix
				, cv::Mat distortionCoefficients) const {
				this->throwIfMissingAConnection<Item::AbstractBoard>();
				auto board = this->getInput<Item::AbstractBoard>();
				if (!board->findBoard(image
					, imagePoints
					, objectPoints
					, findBoardMode
					, cameraMatrix
					, distortionCoefficients)) {
					return false;
				}
				
				auto transform = this->getTransform();

				worldPoints.resize(objectPoints.size());
				for (size_t i = 0; i < objectPoints.size(); i++) {
					ofxCv::toOf(worldPoints[i]) = Utils::applyTransform(transform, ofxCv::toOf(objectPoints[i]));
				}
			}
			//----------
			vector<glm::vec3> BoardInWorld::getAllWorldPoints() const {
				this->throwIfMissingAConnection<Item::AbstractBoard>();
				auto board = this->getInput<Item::AbstractBoard>();
				
				auto allObjectPoints = board->getAllObjectPoints();

				vector<glm::vec3> allWorldPoints;
				allWorldPoints.reserve(allObjectPoints.size());

				auto boardTransform = this->getTransform();

				for (const auto& objectPoint : allObjectPoints) {
					allWorldPoints.push_back(Utils::applyTransform(boardTransform, objectPoint));
				}

				return allWorldPoints;
			}
		}
	}
}

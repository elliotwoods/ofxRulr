#include "pch_RulrNodes.h"
#include "AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			AbstractBoard::AbstractBoard() {

			}

			//----------
			string AbstractBoard::getTypeName() const {
				return "Item::AbstractBoard";
			}

			//----------
			bool AbstractBoard::findBoard(cv::Mat, vector<cv::Point2f> & result, vector<cv::Point3f> & objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const {
				throw(ofxRulr::Exception("Board not implemented"));
			}
		}
	}
}
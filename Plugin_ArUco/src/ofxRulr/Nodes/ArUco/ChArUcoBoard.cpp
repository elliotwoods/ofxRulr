#include "pch_Plugin_ArUco.h"
#include "ChArUcoBoard.h"

using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			ChArUcoBoard::ChArUcoBoard() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string ChArUcoBoard::getTypeName() const {
				return "ArUco::ChArUcoBoard";
			}

			//----------
			void ChArUcoBoard::init() {
				RULR_NODE_UPDATE_LISTENER;
				this->panel = ofxCvGui::Panels::makeImage(this->preview);
				this->manageParameters(this->parameters);
			}

#define INCHES_PER_METER (1.0f / 0.0254f)
			//----------
			void ChArUcoBoard::update() {
				//delete board if it's wrong
				if (this->board) {
					if (this->board->getChessboardSize().width != this->parameters.size.width
						|| this->board->getChessboardSize().height != this->parameters.size.height
						|| this->board->getSquareLength() != this->parameters.squareLength
						|| this->board->getMarkerLength() != this->parameters.markerLength) {
						this->board.release();
						this->dictionary.release();
					}
				}

				//create board if we have none
				if (!this->board) {
					this->dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
					this->board = cv::aruco::CharucoBoard::create(this->parameters.size.width
						, this->parameters.size.height
						, this->parameters.squareLength
						, this->parameters.markerLength
						, dictionary);

					cv::Mat boardImage;
					auto pixelsPerSquare = this->parameters.squareLength.get() * INCHES_PER_METER * this->parameters.previewDPI;

					this->board->draw(cv::Size(this->parameters.size.width * pixelsPerSquare
						, this->parameters.size.height * pixelsPerSquare)
						, boardImage
						, 0);
					ofxCv::copy(boardImage, this->preview.getPixels());
					this->preview.update();
				}
			}

			//----------
			void ChArUcoBoard::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void ChArUcoBoard::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			void ChArUcoBoard::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
			}

			//----------
			//https://github.com/opencv/opencv_contrib/blob/master/modules/aruco/samples/detect_board_charuco.cpp
			bool ChArUcoBoard::findBoard(cv::Mat image, vector<cv::Point2f> & imagePoints, vector<cv::Point3f> & objectPoints, FindBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const {
				if (!this->board) {
					return false;
				}
				else {
					vector<int> markerIds, charucoIds;
					vector<vector<Point2f>> markerCorners, rejectedMarkers;
					vector<Point2f> charucoCorners;
					Vec3d rvec, tvec;

					Ptr<aruco::DetectorParameters> detectorParams = aruco::DetectorParameters::create();
					Ptr<aruco::Board> untypedBoard = this->board.staticCast<aruco::Board>();

					aruco::detectMarkers(image
						, this->dictionary
						, markerCorners
						, markerIds
						, detectorParams
						, rejectedMarkers);

					if (this->parameters.detection.refineStrategy.get()) {
						aruco::refineDetectedMarkers(image
							, this->board
							, markerCorners
							, markerIds
							, rejectedMarkers);
					}

					if (markerIds.empty()) {
						return false;
					}

					auto interpolatedCorners = aruco::interpolateCornersCharuco(markerCorners
						, markerIds
						, image
						, this->board
						, imagePoints
						, charucoIds);

					if (interpolatedCorners == 0) {
						return false;
					}

					//check that all image points are inside image (seems this happens sometimes which results in glitch)
					for (const auto & imagePoint : imagePoints) {
						if (imagePoint.x <= 0
							|| imagePoint.y <= 0
							|| imagePoint.x >= image.cols -1 
							|| imagePoint.y >= image.rows -1) {
							imagePoints.clear();
							return false;
						}
					}

					objectPoints.resize(interpolatedCorners);
					for (int i = 0; i < interpolatedCorners; i++) {
						objectPoints[i] = this->board->chessboardCorners[charucoIds[i]];
					}

					return true;
				}
			}

			//----------
			void ChArUcoBoard::drawObject() const {
				ofPushMatrix();
				{
					ofTranslate(ofVec3f(this->parameters.size.width, this->parameters.size.height, 0) / 2.0f * this->parameters.squareLength);

					auto pixelsPerMeter = this->getPreviewPixelsPerMeter();
					ofScale(1.0f / pixelsPerMeter, 1.0f / pixelsPerMeter, 1.0f);
					this->preview.draw(-this->preview.getWidth() / 2.0f
						, -this->preview.getHeight() / 2.0f
						, this->preview.getWidth()
						, +this->preview.getHeight());
				}
				ofPopMatrix();
			}

			//----------
			float ChArUcoBoard::getSpacing() const {
				return this->parameters.squareLength.get();
			}

			//----------
			ofxCvGui::PanelPtr ChArUcoBoard::getPanel() {
				return this->panel;
			}

			//----------
			float ChArUcoBoard::getPreviewPixelsPerMeter() const {
				return INCHES_PER_METER * this->parameters.previewDPI;
			}
		}
	}
}
#include "pch_Plugin_ArUco.h"
#include "ChArUcoBoard.h"

#define INCHES_PER_METER (1.0f / 0.0254f)

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

				auto panel = ofxCvGui::Panels::makeImage(this->preview);
				panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {

					if (this->parameters.preview.showPaperSizes.get()) {
						//paper sizes
						ofPushMatrix();
						{
							float mmToPixels = 1 / 1000.0f * INCHES_PER_METER * this->parameters.preview.DPI;
							ofScale(mmToPixels, mmToPixels);

							ofColor baseColor(200, 100, 100);
							int index = 0;
							for (auto paperSize : this->paperSizes) {
								auto paperColor = baseColor;
								paperColor.setHue(index * 40);

								ofPushStyle();
								{
									ofSetColor(paperColor);

									//outline
									ofNoFill();
									ofDrawRectangle(0, 0, 297, 210);

									//text
									ofDrawBitmapString(paperSize->name, paperSize->width, paperSize->height);
								}
								ofPopStyle();

								index++;
							}
						}
						ofPopMatrix();
					}
				};
				this->panel = panel;

				this->manageParameters(this->parameters);

				{
					this->paperSizes.emplace(new PaperSize{ "A0", 1189, 841 });
					this->paperSizes.emplace(new PaperSize{ "A1", 841, 594 });
					this->paperSizes.emplace(new PaperSize{ "A2", 594, 420 });
					this->paperSizes.emplace(new PaperSize{ "A3", 420, 297 });
					this->paperSizes.emplace(new PaperSize{ "A4", 297, 210 });
				}
			}

			//----------
			void ChArUcoBoard::update() {
				if (this->parameters.length.marker > this->parameters.length.square) {
					this->parameters.length.marker = this->parameters.length.square;
				}
				//delete board if it's wrong
				if (this->board) {
					if (this->board->getChessboardSize().width != this->parameters.size.width
						|| this->board->getChessboardSize().height != this->parameters.size.height
						|| this->board->getSquareLength() != this->parameters.length.square
						|| this->board->getMarkerLength() != this->parameters.length.marker
						|| this->cachedDPI != this->parameters.preview.DPI) {
						this->board.release();
						this->dictionary.release();
					}
				}

				//create board if we have none
				if (!this->board) {
					bool success = false;
					try {
						this->dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
						this->board = cv::aruco::CharucoBoard::create(this->parameters.size.width
							, this->parameters.size.height
							, this->parameters.length.square
							, this->parameters.length.marker
							, dictionary);

						cv::Mat boardImage;
						auto pixelsPerSquare = this->parameters.length.square.get() * INCHES_PER_METER * this->parameters.preview.DPI;

						this->board->draw(cv::Size(this->parameters.size.width * pixelsPerSquare
							, this->parameters.size.height * pixelsPerSquare)
							, boardImage
							, 0);
						ofxCv::copy(boardImage, this->preview.getPixels());
						this->preview.update();
						this->cachedDPI = this->parameters.preview.DPI;
						success = true;
					}
					RULR_CATCH_ALL_TO_ERROR;
					if (!success) {
						this->preview.clear();
					}
				}
			}

			//----------
			void ChArUcoBoard::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
			}

			//----------
			void ChArUcoBoard::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->parameters);
			}

			//----------
			void ChArUcoBoard::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
			}

			//----------
			bool ChArUcoBoard::findBoard(cv::Mat image
				, vector<cv::Point2f>& imagePoints
				, vector<cv::Point3f>& objectPoints
				, FindBoardMode findBoardMode
				, cv::Mat cameraMatrix
				, cv::Mat distortionCoefficients) const {

				if (findBoardMode == FindBoardMode::Assistant) {
					cv::Rect roi;
					if (!ofxCv::selectROI(image, roi)) {
						return false;
					}
					cv::Mat croppedImage;
					cv::normalize(image(roi)
						, croppedImage
						, 255
						, 0.0
						, cv::NormTypes::NORM_INF);

					if (!this->findBoard(croppedImage
						, imagePoints
						, objectPoints
						, cameraMatrix
						, distortionCoefficients)) {
						return false;
					}
					for (auto& imagePoint : imagePoints) {
						imagePoint.x += roi.x;
						imagePoint.y += roi.y;
					}
					return true;
				}
				else {
					return this->findBoard(image
						, imagePoints
						, objectPoints
						, cameraMatrix
						, distortionCoefficients);
				}
			}

			//----------
			//https://github.com/opencv/opencv_contrib/blob/master/modules/aruco/samples/detect_board_charuco.cpp
			bool ChArUcoBoard::findBoard(cv::Mat image
				, vector<cv::Point2f> & imagePoints
				, vector<cv::Point3f> & objectPoints
				, cv::Mat cameraMatrix
				, cv::Mat distortionCoefficients) const {
				if (!this->board) {
					return false;
				}
				else {
					vector<int> markerIds;
					vector<vector<Point2f>> markerCorners, rejectedMarkers;
					
					cv::Ptr<cv::aruco::Board> untypedBoard = this->board.staticCast<cv::aruco::Board>();

					cv::Ptr<cv::aruco::DetectorParameters> detectorParams = cv::aruco::DetectorParameters::create();
					detectorParams->adaptiveThreshWinSizeMin = 3;
					detectorParams->adaptiveThreshWinSizeMax = 101;
					detectorParams->adaptiveThreshWinSizeStep = 5;
					detectorParams->errorCorrectionRate = this->parameters.detection.errorCorrectionRate;

					if (image.cols > 5000) {
						detectorParams->minMarkerPerimeterRate = 0.003;
					}
					else if (image.cols > 1000) {
						detectorParams->minMarkerPerimeterRate = 0.01;
					}

					//Use OpenCV wrapped version
					cv::aruco::detectMarkers(image
						, this->dictionary
						, markerCorners
						, markerIds
						, detectorParams
						, rejectedMarkers);

					if (this->parameters.detection.refineStrategy.get()) {
						cv::aruco::refineDetectedMarkers(image
							, this->board
							, markerCorners
							, markerIds
							, rejectedMarkers);
					}

					Vec3d rvec, tvec;
					vector<Point2f> charucoCorners;
					vector<int> charucoIds;

					if (markerIds.empty()) {
						return false;
					}

					auto interpolatedCorners = cv::aruco::interpolateCornersCharuco(markerCorners
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
					ofTranslate(ofVec3f(this->parameters.size.width, this->parameters.size.height, 0) / 2.0f * this->parameters.length.square);

					auto pixelsPerMeter = this->getPreviewPixelsPerMeter();
					ofScale(1.0f / pixelsPerMeter, 1.0f / pixelsPerMeter, 1.0f);
					this->preview.draw(-this->preview.getWidth() / 2.0f
						, -this->preview.getHeight() / 2.0f
						, this->preview.getWidth()
						, this->preview.getHeight());
				}
				ofPopMatrix();
			}

			//----------
			float ChArUcoBoard::getSpacing() const {
				return this->parameters.length.square.get();
			}

			//----------
			ofxCvGui::PanelPtr ChArUcoBoard::getPanel() {
				return this->panel;
			}

			//----------
			float ChArUcoBoard::getPreviewPixelsPerMeter() const {
				return INCHES_PER_METER * this->parameters.preview.DPI;
			}
		}
	}
}
#include "pch_RulrNodes.h"
#include "StereoSolvePnP.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/StereoCalibrate.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			//----------
			StereoSolvePnP::StereoSolvePnP() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string StereoSolvePnP::getTypeName() const {
				return "Test::StereoSolvePnP";
			}

			//----------
			void StereoSolvePnP::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Procedure::Calibrate::StereoCalibrate>();
				this->addInput<Item::AbstractBoard>();

				this->manageParameters(this->parameters);
			}

			//----------
			void StereoSolvePnP::update() {
				if (this->isBeingInspected()) {
					try {
						this->throwIfMissingAnyConnection();
						auto stereoCalibrationNode = this->getInput<Procedure::Calibrate::StereoCalibrate>();
						auto boardNode = this->getInput<Item::AbstractBoard>();

						//get the cameras
						stereoCalibrationNode->throwIfACameraIsDisconnected();
						auto cameraNodeA = stereoCalibrationNode->getInput<Item::Camera>("Camera A");
						auto cameraNodeB = stereoCalibrationNode->getInput<Item::Camera>("Camera B");

						//get the frames
						cv::Mat imageA;
						cv::Mat imageB;
						{
							auto frameA = cameraNodeA->getFrame();
							if (!frameA) {
								throw(ofxRulr::Exception("No frame available in Camera A"));
							}
							imageA = ofxCv::toCv(frameA->getPixels());

							auto frameB = cameraNodeB->getFrame();
							if (!frameB) {
								throw(ofxRulr::Exception("No frame available in Camera B"));
							}
							imageB = ofxCv::toCv(frameB->getPixels());
						}

						//find the boards
						vector<cv::Point2f> imagePointsA;
						vector<cv::Point2f> imagePointsB;
						vector<cv::Point3f> objectPoints;
						{
							vector<cv::Point3f> objectPointsA;
							vector<cv::Point3f> objectPointsB;

							auto findBoardMode = this->parameters.findBoardMode.get();
							auto futureA = std::async(std::launch::async, [&] {
								return boardNode->findBoard(imageA
									, imagePointsA
									, objectPointsA
									, findBoardMode
									, cameraNodeA->getCameraMatrix()
									, cameraNodeA->getDistortionCoefficients());
							});
							auto futureB = std::async(std::launch::async, [&] {
								return boardNode->findBoard(imageB
									, imagePointsB
									, objectPointsB
									, findBoardMode
									, cameraNodeB->getCameraMatrix()
									, cameraNodeB->getDistortionCoefficients());
							});

							futureA.get();
							futureB.get();

							Item::AbstractBoard::filterCommonPoints(imagePointsA
								, imagePointsB
								, objectPointsA
								, objectPointsB);

							if (imagePointsA.size() < 3) {
								throw(ofxRulr::Exception("Not enough points seen in both cameras"));
							}

							objectPoints = objectPointsA;
						}

						cv::Mat rotationVector;
						cv::Mat translation;
						stereoCalibrationNode->solvePnP(imagePointsA
							, imagePointsB
							, objectPoints
							, rotationVector
							, translation);

						this->transform = ofxCv::makeMatrix(rotationVector, translation);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			ofxCvGui::PanelPtr StereoSolvePnP::getPanel() {
				return this->panel;
			}

			//----------
			void StereoSolvePnP::drawWorld() {
				ofPushMatrix();
				{
					ofMultMatrix(this->transform);
					ofDrawAxis(0.1f);
				}
				ofPopMatrix();
			}
		}
	}
}
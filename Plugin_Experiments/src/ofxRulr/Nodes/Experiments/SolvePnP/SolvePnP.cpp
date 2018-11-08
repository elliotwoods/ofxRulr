#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolvePnP {
				//----------
				SolvePnP::SolvePnP() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string SolvePnP::getTypeName() const {
					return "Experiments::SolvePnP::SolvePnP";
				}

				//----------
				void SolvePnP::init() {
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;

					this->addInput<ArUco::Detector>();
					this->addInput<ProjectPoints>();
					this->addInput<Item::Camera>();
				}

				//----------
				void SolvePnP::update() {
					auto testObject = this->getInput<TestObject>();
					auto projectPoints = this->getInput<ProjectPoints>();
					auto camera = this->getInput<Item::Camera>();
					if (testObject && projectPoints && camera) {
						auto imagePoints = ofxCv::toCv(projectPoints->getProjectedPoints());
						auto objectPoints = ofxCv::toCv(testObject->getObjectPoints());

						try {
							cv::solvePnP(objectPoints
								, imagePoints
								, camera->getCameraMatrix()
								, camera->getDistortionCoefficients()
								, this->rotationVector
								, this->translation
								, false);

							//get the ground truth
							struct {
								cv::Mat rotationVector, translation;
							} groundTruth;
							testObject->getExtrinsics(groundTruth.rotationVector, groundTruth.translation);

							//get the error
							cv::absdiff(this->rotationVector, groundTruth.rotationVector, this->error.rotationVector);
							cv::absdiff(this->translation, groundTruth.translation, this->error.translation);

							//calculate the moving average
							if (this->movingAverageError.rotationVector.empty()
								|| this->movingAverageError.translation.empty()) {
								this->movingAverageError.rotationVector = this->error.rotationVector.clone();
								this->movingAverageError.translation = this->error.translation.clone();
							}
							else {
								cv::addWeighted(this->movingAverageError.rotationVector, 0.99, this->error.rotationVector, 0.01, 0.0, this->movingAverageError.rotationVector);
								cv::addWeighted(this->movingAverageError.translation, 0.99, this->error.translation, 0.01, 0.0, this->movingAverageError.translation);
							}
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
				}

				//----------
				void SolvePnP::drawWorldStage() {
					try {
						auto transform = ofxCv::makeMatrix(this->rotationVector, this->translation);

						ofPushMatrix();
						{
							ofMultMatrix(transform);
							auto testObject = this->getInput<TestObject>();
							if (testObject) {
								testObject->drawObject();
							}
						}
						ofPopMatrix();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}

				//----------
				void SolvePnP::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;
					
					inspector->addTitle("Result", ofxCvGui::Widgets::Title::Level::H3);
					inspector->addLiveValue<ofVec3f>("Rotation", [this]() {
						if (this->rotationVector.empty()) {
							return ofVec3f();
						}
						else {
							auto value = cv::Point3f(this->rotationVector);
							return ofxCv::toOf(value);
						}
					});
					inspector->addLiveValue<ofVec3f>("Translation", [this]() {
						if (this->translation.empty()) {
							return ofVec3f();
						}
						else {
							auto value = cv::Point3f(this->translation);
							return ofxCv::toOf(value);
						}
					});

					inspector->addTitle("Error", ofxCvGui::Widgets::Title::Level::H3);
					inspector->addLiveValue<ofVec3f>("Rotation", [this]() {
						if (this->error.rotationVector.empty()) {
							return ofVec3f();
						}
						else {
							auto value = cv::Point3f(this->error.rotationVector);
							return ofxCv::toOf(value);
						}
					});
					inspector->addLiveValue<ofVec3f>("Translation", [this]() {
						if (this->error.translation.empty()) {
							return ofVec3f();
						}
						else {
							auto value = cv::Point3f(this->error.translation);
							return ofxCv::toOf(value);
						}
					});

					inspector->addTitle("Error (moving average)", ofxCvGui::Widgets::Title::Level::H3);
					inspector->addLiveValue<ofVec3f>("Rotation", [this]() {
						if (this->movingAverageError.rotationVector.empty()) {
							return ofVec3f();
						}
						else {
							auto value = cv::Point3f(this->movingAverageError.rotationVector);
							return ofxCv::toOf(value);
						}
					});
					inspector->addLiveValue<ofVec3f>("Translation", [this]() {
						if (this->movingAverageError.translation.empty()) {
							return ofVec3f();
						}
						else {
							auto value = cv::Point3f(this->movingAverageError.translation);
							return ofxCv::toOf(value);
						}
					});
				}
			}
		}
	}
}
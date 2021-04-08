#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolvePnP {
				//----------
				ProjectPoints::ProjectPoints() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string ProjectPoints::getTypeName() const {
					return "Experiments::SolvePnP::ProjectPoints";
				}

				//----------
				void ProjectPoints::init() {
					RULR_NODE_UPDATE_LISTENER;

					this->addInput<TestObject>();
					this->addInput<Item::Camera>();
					this->manageParameters(this->parameters);

					this->panel = make_shared<ofxCvGui::Panels::Draws>(this->fbo);
					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments &) {
						for (const auto & projectedPoint : this->projectedPoints) {
							ofPushMatrix();
							{
								ofTranslate(projectedPoint);

								ofPushStyle();
								{
									//draw circle
									ofNoFill();
									ofSetColor(255, 0, 0);
									ofDrawCircle(glm::vec2(), 5.0f);

									//draw cross
									ofRotateDeg(45.0f);
									ofDrawLine(-5, 0, 5, 0);
									ofDrawLine(0, -5, 0, 5);
								}
								ofPopStyle();
							}
							ofPopMatrix();
						}
					};

					panel->onDraw += [this](ofxCvGui::DrawArguments & args) {
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							if (camera->getTransform() != glm::mat4(1.0f)) {
								ofxCvGui::Utils::drawText("Camera must have zero transform"
									, args.localBounds
									, true);
							}
						}
					};
				}

				//----------
				void ProjectPoints::update() {
					auto testObject = this->getInput<TestObject>();
					auto camera = this->getInput<Item::Camera>();
					if (camera && testObject) {
						//allocate the fbo if unavailable
						if (this->fbo.getWidth() != camera->getWidth()
							|| this->fbo.getHeight() != camera->getHeight()) {
							this->fbo.allocate(camera->getWidth(), camera->getHeight(), GL_RGBA);
						}

						auto viewCamera = camera->getViewInWorldSpace();

						//draw into the fbo
						this->fbo.begin();
						{
							ofClear(0, 0);
							viewCamera.beginAsCamera();
							{
								testObject->drawWorldStage();
							}
							viewCamera.endAsCamera();
						}
						this->fbo.end();

						//project the points
						{
							auto cameraMatrix = camera->getCameraMatrix();
							auto distortionCoefficients = camera->getDistortionCoefficients();

							cv::Mat rotation, translation;
							testObject->getExtrinsics(rotation, translation);

							cv::projectPoints(ofxCv::toCv(testObject->getObjectPoints())
								, rotation
								, translation
								, cameraMatrix
								, distortionCoefficients
								, ofxCv::toCv(this->projectedPoints));
						}

						//apply the noise
						{
							for (auto & point : this->projectedPoints) {
								point.x += ofRandomf() * this->parameters.noise;
								point.y += ofRandomf() * this->parameters.noise;
							}
						}
					}
				}

				//----------
				ofxCvGui::PanelPtr ProjectPoints::getPanel() {
					return this->panel;
				}

				//----------
				const std::vector<glm::vec2> & ProjectPoints::getProjectedPoints() {
					return this->projectedPoints;
				}
			}
		}
	}
}
#include "pch_Plugin_Scrap.h"
#include "ProjectCircle.h"


#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/CircleLaser.h"
#include "ofxRulr/Nodes/Procedure/Calibrate/FindLine.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//---------
				ProjectCircle::ProjectCircle() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string ProjectCircle::getTypeName() const {
					return "Procedure::Calibrate::ProjectCircle";
				}

				//---------
				void ProjectCircle::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Item::Camera>();
					this->addInput<Item::CircleLaser>();
					this->addInput<Procedure::Calibrate::FindLine>();

					auto panel = ofxCvGui::Panels::makeImage(this->preview);

					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						ofPushStyle();
						{
							ofNoFill();
							ofSetLineWidth(2.0f);
							ofSetCircleResolution(100);
							ofDrawCircle(this->parameters.circle.position.x
								, this->parameters.circle.position.y
								, this->parameters.circle.radius);
						}
						ofPopStyle();

						if (this->calibration) {
							ofPushStyle();
							{
								for (int i = 0; i < 3; i++) {
									ofDrawLine(this->calibration->previewImageExtremities[i].first
										, this->calibration->previewImageExtremities[i].second);
									ofDrawCircle(this->calibration->previewImageExtremities[i].first, 5);
								}

								ofSetColor(100, 100, 100);
								for (int i = 0; i < 3; i++) {
									ofDrawLine(this->calibration->previewImageCircleLines[i].first
										, this->calibration->previewImageCircleLines[i].second);
									ofDrawCircle(this->calibration->previewImageCircleLines[i].first, 5);
								}

								ofNoFill();
								ofSetColor(100, 100, 255);
								ofDrawCircle(this->calibration->center, 10.0f);
							}
							ofPopStyle();
						}
					};

					auto panelWeak = weak_ptr<ofxCvGui::Panels::Image>(panel);
					panel->onMouse += [this, panelWeak](ofxCvGui::MouseArguments & args) {
						auto panel = panelWeak.lock();
						if (args.action == ofxCvGui::MouseArguments::Action::Dragged && this->parameters.circle.enableDragging) {
							auto movement = args.movement / panel->getZoomFactor();
							if (args.button == 0) {
								this->parameters.circle.position.x += movement.x / 2.0f;
								this->parameters.circle.position.y += movement.y / 2.0f;
							}
							else {
								this->parameters.circle.radius += (movement.y + movement.x) / 2.0f;
							}
						}
					};

					this->panel = panel;
				}

				//---------
				void ProjectCircle::update() {
					auto camera = this->getInput<Item::Camera>();
					if (camera) {
						if (camera->getGrabber()->isFrameNew()) {
							this->preview = camera->getGrabber()->getPixels();
							this->preview.update();
						}
					}
				}

				//---------
				ofxCvGui::PanelPtr ProjectCircle::getPanel() {
					return this->panel;
				}

				//---------
				void ProjectCircle::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);
				}

				//---------
				void ProjectCircle::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);
				}

				//---------
				void ProjectCircle::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addParameterGroup(this->parameters);

					inspector->addButton("Draw circle", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Draw circle");
							this->calibrateAndDrawCircle();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
				}

				//---------
				void ProjectCircle::calibrateAndDrawCircle() {
					this->throwIfMissingAnyConnection();

					auto circleLaser = this->getInput<Item::CircleLaser>();
					auto camera = this->getInput<Item::Camera>();
					auto findLine = this->getInput<Procedure::Calibrate::FindLine>();

					auto range = this->parameters.fitting.range.get();

					struct DataPoint {
						double laserAngle;
						cv::Vec4f imageLine;
					};

					auto imageLineToRay = [](const cv::Vec4f & imageLine) {
						return ofxRay::Ray(ofVec2f(imageLine[2], imageLine[3])
							, ofVec2f(imageLine[0], imageLine[1]));
					};

					auto performScan = [&](const vector<double> & laserAngles) {
						Utils::ScopedProcess scopedProcess("Perform scan", false, laserAngles.size());

						vector<DataPoint> dataPoints;

						for (const auto & laserAngle : laserAngles) {
							Utils::ScopedProcess scopedProcessLaserAngle("Angle " + ofToString(laserAngle), false);

							if (laserAngle < -1 || laserAngle > +1) {
								throw(ofxRulr::Exception("Laser angle outside range (" + ofToString(laserAngle) + ")"));
							}
							circleLaser->drawPoint(ofVec2f(laserAngle, this->parameters.laserY));

							auto cameraFrameForeground = camera->getFreshFrame();
							if (!cameraFrameForeground) {
								throw(ofxRulr::Exception("Failed to capture camera frame"));
							}

							circleLaser->clearDrawing();

							auto cameraFrameBackground = camera->getFreshFrame();
							if (!cameraFrameBackground) {
								throw(ofxRulr::Exception("Failed to capture camera frame"));
							}

							auto lineResult = findLine->findLine(cameraFrameForeground->getPixels(), cameraFrameBackground->getPixels());

							if (lineResult->line[1] > 0) {
								lineResult->line[0] *= -1;
								lineResult->line[1] *= -1;
							}

							dataPoints.push_back(DataPoint{
								laserAngle
								, lineResult->line
							});
						}

						return dataPoints;
					};

					this->calibration = make_unique<Calibration>();

					auto calibrate = [&](const vector<DataPoint> & dataPoints) {
						vector<ofVec2f> laserAnglesToResultAngles; //x

						for (const auto & dataPoint : dataPoints) {
							laserAnglesToResultAngles.emplace_back(atan2(dataPoint.imageLine[1], dataPoint.imageLine[0])
								, dataPoint.laserAngle);
						}

						this->calibration->polyFit = ofxRulr::Utils::PolyFit::fit(laserAnglesToResultAngles, laserAnglesToResultAngles.size() - 1);
					};

					{
						Utils::ScopedProcess scopedProcessFirstScan("First scan");

						//setup and scan first range
						vector<DataPoint> firstScan;
						{
							vector<double> firstPoints = { -range, 0, +range };
							firstScan = performScan(firstPoints);
						}

						//find center point
						{
							//find the origin point using the outer rays
							auto rayLeft = imageLineToRay(firstScan[0].imageLine);
							auto rayRight = imageLineToRay(firstScan[1].imageLine);

							this->calibration->center = rayLeft.intersect(rayRight).getMidpoint();
						}

						//build preview lines
						{
							for (int i = 0; i < 3; i++) {
								this->calibration->previewImageExtremities[i].first = ofVec2f(firstScan[i].imageLine[2]
									, firstScan[i].imageLine[3]);
								this->calibration->previewImageExtremities[i].second = ofVec2f(firstScan[i].imageLine[0]
									, firstScan[i].imageLine[1]) * 1000.0f + this->calibration->previewImageExtremities[i].first;
							}
						}

						//perform first fit
						{
							calibrate(firstScan);
						}

						scopedProcessFirstScan.end();
					}

					//calculate the intended angles (tangents)
					vector<double> targetImageAngles;
					{
						auto circleCenter = ofVec2f(this->parameters.circle.position.x
							, this->parameters.circle.position.y);
						auto radius = this->parameters.circle.radius;


						auto delta = circleCenter - this->calibration->center;
						this->calibration->angleCenter = atan2(delta.y, delta.x);

						auto distance = delta.length();
						this->calibration->tangentAngle = asin(radius / distance);

						targetImageAngles = {
							calibration->angleCenter - calibration->tangentAngle
							, calibration->angleCenter
							, calibration->angleCenter + calibration->tangentAngle };
					}

					{
						Utils::ScopedProcess scopedProcessSecondScan("Second scan");

						//setup and scan second range
						vector<DataPoint> secondScan;
						{
							//calculate angles in laser space
							vector<double> secondRoundLaserAngles;
							for (const auto & imageAngle : targetImageAngles) {
								auto laserAngle = ofxRulr::Utils::PolyFit::evaluate(calibration->polyFit, imageAngle);
								secondRoundLaserAngles.push_back(laserAngle);
							}

							secondScan = performScan(secondRoundLaserAngles);
							calibrate(secondScan);
						}

						scopedProcessSecondScan.end();
					}

					//draw the circle
					{
						auto angleLeft = ofxRulr::Utils::PolyFit::evaluate(calibration->polyFit, targetImageAngles[0]);
						auto angleRight = ofxRulr::Utils::PolyFit::evaluate(calibration->polyFit, targetImageAngles[2]);

						const auto center = ofVec2f((angleLeft + angleRight) / 2.0f, this->parameters.laserY);
						const auto radius = abs(angleRight - angleLeft) / 2.0f;

						cout << "Drawing circle" << endl;
						cout << "center = " << center << endl;
						cout << "radius = " << radius << endl;

						circleLaser->drawCircle(center, radius);
					}
				}
			}
		}
	}
}
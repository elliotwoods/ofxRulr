#include "pch_MultiTrack.h"

#ifdef OFXMULTITRACK_TCP

#include "FindMarker.h"

#include "../Subscriber.h"

using namespace ofxCvGui;
using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			namespace Test {
				//----------
				FindMarker::FindMarker() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				cv::string FindMarker::getTypeName() const {
					return "MultiTrack::Test::FindMarker";
				}

				//----------
				void FindMarker::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					this->addInput<Subscriber>();

					auto panel = ofxCvGui::Panels::Groups::makeStrip();
					panel->add(Panels::makeTexture(this->infrared, "Infrared"));
					auto resultPanel = Panels::makeImage(this->threshold, "Threshold");
					panel->add(resultPanel);
					this->panel = panel;

					resultPanel->onDrawImage += [this](DrawImageArguments & args) {
						int index = 0;
						for (const auto & marker : this->markers) {
							ofPushStyle();
							{
								ofNoFill();
								ofSetLineWidth(1.0f);

								ofSetColor(ofColor::white);
								marker.outline.draw();

								ofSetColor(255, 100, 100);
								ofDrawCircle(marker.center, marker.radius);

								ofDrawBitmapStringHighlight(ofToString(index), marker.center);
							}
							ofPopStyle();

							index++;
						}
					};
				}

				//----------
				void FindMarker::update() {
					if (!this->parameters.enabled) {
						return;
					}

					auto subscriberNode = this->getInput<Subscriber>();
					if (subscriberNode) {
						auto subscriber = subscriberNode->getSubscriber();
						if (subscriber) {
							if (subscriber->isFrameNew()) {
								try {
									auto & frame = subscriber->getFrame();

									const auto & infrared = frame.getInfrared();
									this->infrared.loadData(infrared);

									auto infraRed16 = infrared; //copy local
									auto infraRed16Mat = toCv(infraRed16);

									auto infraRed8Mat = infraRed16Mat.clone();
									infraRed8Mat.convertTo(infraRed8Mat, CV_8U, 1.0f / float(1 << 8));

									cv::threshold(infraRed8Mat, infraRed8Mat, this->parameters.threshold, 255, THRESH_TOZERO);
									ofxCv::copy(infraRed8Mat, this->threshold);
									this->threshold.update();

									vector<vector<cv::Point2i>> contours;
									cv::findContours(infraRed8Mat, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

									this->markers.clear();
									for (auto & contour : contours) {
										if (cv::contourArea(contour) >= this->parameters.minimumArea) {
											Marker marker;
											for (auto point : contour) {
												marker.outline.addVertex(toOf(point));
											}
											marker.outline.close();

											cv::minEnclosingCircle(contour, toCv(marker.center), marker.radius);

											this->markers.push_back(marker);
										}
									}
								}
								RULR_CATCH_ALL_TO_ERROR;
							}
						}
					}
				}

				//----------
				ofxCvGui::PanelPtr FindMarker::getPanel() {
					return this->panel;
				}

				//----------
				void FindMarker::drawWorld() {

				}

				//----------
				void FindMarker::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;
					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void FindMarker::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);
				}

				//----------
				void FindMarker::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);
				}
			}
		}
	}
}

#endif // OFXMULTITRACK_TCP
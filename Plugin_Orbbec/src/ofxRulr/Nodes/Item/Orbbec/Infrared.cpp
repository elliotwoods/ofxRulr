#include "pch_Plugin_Orbbec.h"
#include "Infrared.h"

using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			namespace Orbbec {
				//----------
				Infrared::Infrared() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Infrared::getTypeName() const {
					return "Item::Orbbec::Infrared";
				}

				//----------
				void Infrared::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;

					this->addInput<Device>();

					{
						auto panel = ofxCvGui::Panels::makeTexture(this->texture);
						auto style = make_shared<ofxCvGui::Panels::Texture::Style>();
						style->rangeMaximum = float(1 << 10) / float(1 << 16);
						panel->setStyle(style);
						this->panel = panel;
					}

					//default device settings
					this->setWidth(640);
					this->setHeight(480);
					this->focalLengthX.set(570);
					this->focalLengthY.set(570);
					this->principalPointX.set(320);
					this->principalPointY.set(240);

					this->onDrawObject += [this]() {
						if (this->texture.isAllocated()) {
							this->getViewInObjectSpace().drawOnNearPlane(this->texture);
						}
					};
				}

				//----------
				void Infrared::update() {
					auto deviceNode = this->getInput<Device>();
					if (deviceNode) {
						//sync location with device
						this->setTransform(deviceNode->getTransform());

						auto device = deviceNode->getDevice();
						if (device) {
							if (device->isFrameNew()) {
								auto infraredStream = device->get<ofxOrbbec::Streams::Infrared>(false);
								if (infraredStream) {
									this->setWidth(infraredStream->getWidth());
									this->setHeight(infraredStream->getHeight());
									this->texture = infraredStream->getTexture();
								}
							}
						}
					}
				}

				//----------
				ofxCvGui::PanelPtr Infrared::getPanel() {
					return this->panel;
				}

				//----------
				void Infrared::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;
					inspector->addTitle("Calibrate");
					{
						inspector->addButton("Calculate intrinsics", [this]() {
							try {
								ofxRulr::Utils::ScopedProcess scopedProcess("Calibrating depth intrinsics from depth/world correspondences");
								this->calcIntrinsicsUsingDepthToWorld();
								scopedProcess.end();
							}
							RULR_CATCH_ALL_TO_ALERT;
						});
						inspector->addLiveValue<float>("Reprojection error", [this]() {
							return this->results.reprojectionError;
						});
					}
					
				}

				//----------
				void Infrared::serialize(Json::Value & json) {
					json["results"]["reprojectionError"] = this->results.reprojectionError;
				}

				//----------
				void Infrared::deserialize(const Json::Value & json) {
					this->results.reprojectionError = json["results"]["reprojectionError"].asFloat();
				}

				//----------
				void Infrared::calcIntrinsicsUsingDepthToWorld() {
					this->throwIfMissingAConnection<Device>();
					
					auto deviceNode = this->getInput<Device>();
					auto device = deviceNode->getDevice();
					if (!device) {
						throw(ofxRulr::Exception("Device not initialised"));
					}
					
					auto depth = device->getDepth();
					if (!depth) {
						throw(ofxRulr::Exception("Depth stream not initialised"));
					}

					//synthesise depth points
					vector<ofVec3f> depthPoints;
					vector<ofVec2f> imagePoints;
					{
						for (auto x = 0; x < 640; x += 80) {
							for (auto y = 0; y <= 480; y += 80) {
								for (auto z = 1000; z < 5000; z += 500) {
									depthPoints.emplace_back(ofVec3f(x, y, z));
									imagePoints.emplace_back(ofVec2f(x, y));
								}
							}
						}
					}

					//synthesise world points
					vector<ofVec3f> worldPoints;
					for (const auto depthPoint : depthPoints) {
						worldPoints.emplace_back(depth->depthToWorld(depthPoint) * ofVec3f(1, 1, 1));
					}
					
					//calibrate the camera on this data
					{
						cv::Mat cameraMatrix = this->getCameraMatrix();
						auto distortionCoefficients = this->getDistortionCoefficients();

						int flags = CV_CALIB_FIX_K5 | CV_CALIB_FIX_K6 | CV_CALIB_USE_INTRINSIC_GUESS;

						vector<vector<ofVec3f>> worldPoints2(1, worldPoints);
						vector<vector<ofVec2f>> imagePoints2(1, imagePoints);
						vector<cv::Mat> rotations, translations;
						cout << cameraMatrix << endl;
						cout << this->getSize() << endl;
						this->results.reprojectionError = cv::calibrateCamera(toCv(worldPoints2), toCv(imagePoints2), this->getSize(), cameraMatrix, distortionCoefficients, rotations, translations, flags);

						this->setIntrinsics(cameraMatrix, distortionCoefficients);
					}
				}
			}
		}
	}
}
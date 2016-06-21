#include "pch_Plugin_KinectForWindows2.h"

#include "CameraWithKinectV2.h"

#include "ofxRulr/Nodes/Procedure/Calibrate/CameraFromKinectV2.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/KinectV2.h"

#include "ofxRulr/Utils/Constants.h"

#include "ofxCvGui/Widgets/MultipleChoice.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			//----------
			CameraWithKinectV2::CameraWithKinectV2() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string CameraWithKinectV2::getTypeName() const {
				return "Test::CameraWithKinectV2";
			}

			//----------
			void CameraWithKinectV2::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				
				this->addInput(MAKE(ofxRulr::Graph::Pin<Procedure::Calibrate::CameraFromKinectV2>));

				this->activewhen.set("Active when", 0, 0, 1);
			}

			//----------
			void CameraWithKinectV2::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				auto activeWhenWidget = new Widgets::MultipleChoice("Active", { "When Selected", "Always" });
				activeWhenWidget->entangle(this->activewhen);
				inspector->add(activeWhenWidget);
			}

			//----------
			void CameraWithKinectV2::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->activewhen);
			}

			//----------
			void CameraWithKinectV2::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->activewhen);
			}

			//----------
			void CameraWithKinectV2::drawWorld() {
				if (this->activewhen == 0 && !this->isBeingInspected()) return;

				auto calibrate = this->getInput<Procedure::Calibrate::CameraFromKinectV2>();
				if (!calibrate) return;

				auto cameraNode = calibrate->getInput<Item::Camera>();
				auto kinectNode = calibrate->getInput<Item::KinectV2>();
				if (!cameraNode || !kinectNode) return;

				bool frameNew = cameraNode->getGrabber()->isFrameNew();
				if (kinectNode->getDevice()->isFrameNew()) {
					if (!this->depthToWorldTable.isAllocated()) {
						kinectNode->getDevice()->getDepthSource()->getDepthToWorldTable(this->depthToWorldTable);
					}
					frameNew = true;
				}

				if (!frameNew || !this->depthToWorldTable.isAllocated()) return;

				ofPushStyle();
				{
					ofPushMatrix();
					{
						auto depthSource = kinectNode->getDevice()->getDepthSource();
						auto depthPixel = depthSource->getPixels().getData();
						auto depthToWorldRay = (ofVec2f *)this->depthToWorldTable.getData();

						auto colorPixels = cameraNode->getGrabber()->getPixels();
						const vector<float> distortion = cameraNode->getDistortionCoefficients();
						const auto cameraRay = cameraNode->getViewInWorldSpace(); 
						const auto viewProjection = cameraRay.getViewMatrix() * cameraRay.getClippedProjectionMatrix();
						
						ofMesh mesh;
						const auto size = depthSource->getWidth() * depthSource->getHeight();
						for (int i = 0; i < size; ++i) {
							auto z = (float)*depthPixel / 1000.0f;

							ofVec3f vertex{
								depthToWorldRay->x * z,
								depthToWorldRay->y * z,
								z
							};
							mesh.addVertex(vertex);

							if (z > 0.1) {
								auto colorPos = vertex * viewProjection;
								//cout << "Depth point " << i << " has color pos " << colorPos << endl;
								if (distortion.size() >= 4) {
									colorPos = this->getUndistorted(distortion, colorPos / 2.0f) * 2.0f;
								}
								auto colorX = (int)((1.0f + colorPos.x) / 2.0f * colorPixels.getWidth());
								auto colorY = (int)((1.0f - colorPos.y) / 2.0f * colorPixels.getHeight());

								if (colorX >= 0 && colorX < colorPixels.getWidth() && colorY >= 0 && colorY < colorPixels.getHeight()) {
									mesh.addColor(colorPixels.getColor(colorX, colorY));
								}
								else {
									mesh.addColor(ofColor::red);
								}
							}
							else {
								mesh.addColor(ofColor::white);
							}

							depthPixel++;
							depthToWorldRay++;
						}
						mesh.draw(OF_MESH_POINTS);
					}
					ofPopMatrix();
				}
				ofPopStyle();
			}

			//--------------------------------------------------------------
			ofVec2f CameraWithKinectV2::getUndistorted(const vector<float> & distortion, const ofVec2f & point)
			{
				float lengthSquared = point.x * point.x + point.y * point.y;

				float a = 1.0f + lengthSquared * (distortion[0] + distortion[1] * lengthSquared);
				float b = 2.0f * point.x * point.y;

				ofVec2f result;
				result.x = a * point.x + b * distortion[2] + distortion[3] * (lengthSquared + 2.0f * point.x * point.y);
				result.y = a * point.y + distortion[2] * (lengthSquared + 2.0f * point.y * point.y) + b * distortion[3];

				return result;
			}
		}
	}
}
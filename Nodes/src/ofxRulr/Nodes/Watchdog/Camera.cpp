#include "pch_RulrNodes.h"
#include "Camera.h"

#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Watchdog {
			//----------
			Camera::Camera()
			: lastCaptureTime(chrono::high_resolution_clock::now()) {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Camera::getTypeName() const {
				return "Watchdog::Camera";
			}

			//----------
			void Camera::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				auto cameraInput = this->addInput<Item::Camera>();

				cameraInput->onNewConnection += [this](shared_ptr<Item::Camera> cameraNode) {
					//reset the timer (otherwise we would be referencing an old time before we started watching)
					this->lastCaptureTime.store(chrono::high_resolution_clock::now());

					cameraNode->onNewFrame.addListener([this](shared_ptr<ofxMachineVision::Frame> frame) {
						this->lastCaptureTime.store(chrono::high_resolution_clock::now());
					}, this);
				};
				cameraInput->onDeleteConnection += [this](shared_ptr<Item::Camera> cameraNode) {
					if (cameraNode) {
						cameraNode->onNewFrame.removeListeners(this);
					}
				};

				this->manageParameters(this->parameters);
			}

			//----------
			void Camera::update() {
				if (!this->getInputPin<Item::Camera>()->isConnected()) {
					return;
				}
				if (this->parameters.reopnWhenNoNewFrames.enabled) {
					auto timeSinceLastFrame = chrono::high_resolution_clock::now() - this->lastCaptureTime.load();
					auto timeout = chrono::duration<float, ratio<1, 1>>(this->parameters.reopnWhenNoNewFrames.timeout.get());
					if (timeSinceLastFrame > timeout) {
						//then reopen the camera
						auto cameraNode = this->getInput<Item::Camera>();
						cameraNode->getGrabber()->reopen();
					}
				}

				if (this->parameters.rebootWhenNoNewFrames.enabled) {
					auto timeSinceLastFrame = chrono::high_resolution_clock::now() - this->lastCaptureTime.load();
					auto timeout = chrono::duration<float, ratio<1, 1>>(this->parameters.rebootWhenNoNewFrames.timeout.get());
					if (timeSinceLastFrame > timeout) {
#ifdef TARGET_WIN32
						system("shutdown /r /t 5");
#elif TARGET_OSX
						system("sudo reboot");
#endif
					}
				}
			}

			//----------
			void Camera::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addLiveValue<string>("Time since last capture", [this]() {
					return Utils::formatDuration(chrono::high_resolution_clock::now() - this->lastCaptureTime.load());
				});
			}
		}
	}
}
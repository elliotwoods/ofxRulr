#include "Camera.h"
#include "ofxCvGui.h"

using namespace ofxCvGui;
using namespace ofxMachineVision;

#define CHECK_GRABBER if(!this->grabber) return;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Camera::Camera() {
			this->exposure.set("Exposure", 500.0f, 0.0f, 1000.0f);
			this->gain.set("Gain", 0.5f, 0.0f, 1.0f);
			this->focus.set("Focus ", 0.5f, 0.0f, 1.0f);
			this->sharpness.set("Sharpness", 0.5f, 0.0f, 1.0f);

			this->exposure.addListener(this, & Camera::exposureCallback);
			this->gain.addListener(this, & Camera::gainCallback);
			this->focus.addListener(this, & Camera::focusCallback);
			this->sharpness.addListener(this, & Camera::sharpnessCallback);
		}

		//----------
		string Camera::getTypeName() const {
			return "Camera";
		}

		//----------
		void Camera::update() {
			CHECK_GRABBER
			this->grabber->update();
		}

		//----------
		void Camera::populate(ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make("Camera", Widgets::Title::Level::H2));

			inspector->add(Widgets::LiveValueHistory::make("Framerate [Hz]", [this] () {
				if (this->grabber) {
					return this->grabber->getFps();
				} else {
					return 0.0f;
				}
			}, true));
			inspector->add(Widgets::Slider::make(this->exposure));
			inspector->add(Widgets::Slider::make(this->gain));
			inspector->add(Widgets::Slider::make(this->focus));
			inspector->add(Widgets::Slider::make(this->sharpness));
		}

		//----------
		void Camera::setDevice(DevicePtr device, int deviceIndex) {
			this->grabber = shared_ptr<Grabber::Simple>(new Grabber::Simple(device));
			this->grabber->open(deviceIndex);
			this->grabber->startCapture();
			this->grabber->setExposure(this->exposure);
			this->grabber->setGain(this->gain);
			this->grabber->setFocus(this->focus);
			this->grabber->setSharpness(this->sharpness);
		}

		//----------
		shared_ptr<Grabber::Simple> Camera::getGrabber() {
			return this->grabber;
		}

		//----------
		void Camera::exposureCallback(float & value) {
			CHECK_GRABBER
			this->grabber->setExposure(value);
		}

		//----------
		void Camera::gainCallback(float & value) {
			CHECK_GRABBER
			this->grabber->setGain(value);
		}

		//----------
		void Camera::focusCallback(float & value) {
			CHECK_GRABBER
			this->grabber->setFocus(value);
		}

		//----------
		void Camera::sharpnessCallback(float & value) {
			CHECK_GRABBER
			this->grabber->setSharpness(value);
		}
	}
}
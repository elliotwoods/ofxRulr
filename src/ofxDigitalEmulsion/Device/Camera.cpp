#include "Camera.h"
#include "ofxCvGui.h"

using namespace ofxCvGui;
using namespace ofxMachineVision;

namespace ofxDigitalEmulsion {
	namespace Device {
		//----------
		Camera::Camera() {
			this->setDevice(DevicePtr(new ofxMachineVision::Device::VideoInputDevice()));

			this->exposure.set("Exposure", 500.0f, 0.0f, 1000.0f);
			this->gain.set("Gain", 0.5f, 0.0f, 1.0f);
			this->focus.set("Focus", 0.5f, 0.0f, 1.0f);

			this->exposure.addListener(this, & Camera::exposureCallback);
			this->gain.addListener(this, & Camera::gainCallback);
			this->focus.addListener(this, & Camera::focusCallback);
		}

		//----------
		string Camera::getTypeName() const {
			return "Camera";
		}

		//----------
		void Camera::update() {
			if (!this->grabber) return;
			this->grabber->update();
		}

		//----------
		void Camera::populate(ElementGroupPtr inspector) {
			inspector->add(Widgets::makeTitle("Camera", Widgets::Title::Level::H2));

			inspector->add(Widgets::make(this->exposure));
			inspector->add(Widgets::make(this->gain));
			inspector->add(Widgets::make(this->focus));
		}

		//----------
		void Camera::setDevice(DevicePtr device, int deviceIndex) {
			this->grabber = shared_ptr<Grabber::Simple>(new Grabber::Simple(device));
			this->grabber->open(deviceIndex);
			this->grabber->startCapture();
			this->grabber->setExposure(this->exposure);
			this->grabber->setGain(this->gain);
			this->grabber->setFocus(this->focus);
		}

		//----------
		shared_ptr<Grabber::Simple> Camera::getGrabber() {
			return this->grabber;
		}

		//----------
		void Camera::exposureCallback(float & value) {
			this->grabber->setExposure(value);
		}

		//----------
		void Camera::gainCallback(float & value) {
			this->grabber->setGain(value);
		}

		//----------
		void Camera::focusCallback(float & value) {
			this->grabber->setFocus(value);
		}
	}
}
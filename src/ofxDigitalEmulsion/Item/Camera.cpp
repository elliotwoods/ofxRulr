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
		ofxCvGui::PanelPtr Camera::getView() {
			auto view = ofxCvGui::Builder::makePanel(this->grabber->getTextureReference(), this->getTypeName());
			
			view->onDraw += [this] (ofxCvGui::DrawArguments &) {
				stringstream status;
				status << "Device ID : " << this->getGrabber()->getDeviceID() << endl;
				status << endl;
				status << this->getGrabber()->getDeviceSpecification().toString() << endl;
			
				ofDrawBitmapStringHighlight(status.str(), 20, 80, ofColor(0x46), ofColor::white);
			};

			return view;
		}

		//----------
		void Camera::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->exposure, json);
			Utils::Serializable::serialize(this->gain, json);
			Utils::Serializable::serialize(this->focus, json);
			Utils::Serializable::serialize(this->sharpness, json);
		}

		//----------
		void Camera::deserialize(Json::Value & json) {
			Utils::Serializable::deserialize(this->exposure, json);
			Utils::Serializable::deserialize(this->gain, json);
			Utils::Serializable::deserialize(this->focus, json);
			Utils::Serializable::deserialize(this->sharpness, json);
			
			this->grabber->setExposure(this->exposure);
			this->grabber->setGain(this->gain);
			this->grabber->setFocus(this->focus);
			this->grabber->setSharpness(this->sharpness);
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
		void Camera::populateInspector2(ElementGroupPtr inspector) {
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
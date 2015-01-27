#include "Camera.h"
#include "ofxCvGui.h"

using namespace ofxCvGui;
using namespace ofxMachineVision;
using namespace cv;

#define CHECK_GRABBER if(!this->grabber) return;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Camera::Camera() {
			this->showSpecification.set("Show specification", false);
			this->showFocusLine.set("Show focus line", true);
			this->exposure.set("Exposure [us]", 500.0f, 0.0f, 1000000.0f);
			this->gain.set("Gain", 0.5f, 0.0f, 1.0f);
			this->focus.set("Focus ", 0.5f, 0.0f, 1.0f);
			this->sharpness.set("Sharpness", 0.5f, 0.0f, 1.0f);

			this->focalLengthX.set("Focal Length X", 1024.0f, 1.0f, 50000.0f);
			this->focalLengthY.set("Focal Length Y", 1024.0f, 1.0f, 50000.0f);
			this->principalPointX.set("Center Of Projection X", 512.0f, -10000.0f, 10000.0f);
			this->principalPointY.set("Center Of Projection Y", 512.0f, -10000.0f, 10000.0f);
			for(int i=0; i<OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT; i++) {
				this->distortion[i].set("Distortion K" + ofToString(i + 1), 0.0f, -1000.0f, 1000.0f);
			}

			this->translationX.set("Translation X", 0.0f, -100.0f, 100.0f);
			this->translationY.set("Translation Y", 0.0f, -100.0f, 100.0f);
			this->translationZ.set("Translation Z", 0.0f, -100.0f, 100.0f);
			this->rotationX.set("Rotation X", 0.0f, -360.0f, 360.0f);
			this->rotationY.set("Rotation Y", 0.0f, -360.0f, 360.0f);
			this->rotationZ.set("Rotation Z", 0.0f, -360.0f, 360.0f);

			this->exposure.addListener(this, & Camera::exposureCallback);
			this->gain.addListener(this, & Camera::gainCallback);
			this->focus.addListener(this, & Camera::focusCallback);
			this->sharpness.addListener(this, & Camera::sharpnessCallback);
		}

		//----------
		void Camera::init() {
			if (!this->grabber) {
				//throw(ofxDigitalEmulsion::Utils::Exception("Cannot initialise [Camera] node, no camera device has been set."));
				this->grabber = make_shared<ofxMachineVision::Webcam>();
				this->grabber->open();
				this->grabber->startCapture();
			}
			auto view = ofxCvGui::Builder::makePanel(this->grabber->getTextureReference(), this->getTypeName());
			view->onDraw += [this](ofxCvGui::DrawArguments & args) {
				if (this->showSpecification) {
					stringstream status;
					status << "Device ID : " << this->getGrabber()->getDeviceID() << endl;
					status << endl;
					status << this->getGrabber()->getDeviceSpecification().toString() << endl;

					ofDrawBitmapStringHighlight(status.str(), 30, 90, ofColor(0x46, 200), ofColor::white);
				}
			};
			view->onDrawCropped += [this](Panels::BaseImage::DrawCroppedArguments & args) {
				if (this->showFocusLine) {
					ofPushMatrix();
					ofPushStyle();

					ofTranslate(0, args.size.y / 2.0f);
					ofSetColor(100, 255, 100);
					ofLine(0, 0, args.size.x, 0);

					ofTranslate(0, +128);
					ofScale(1.0f, -1.0f);
					ofSetColor(0, 0, 0);
					ofSetLineWidth(2.0f);
					this->focusLineGraph.draw();
					ofSetLineWidth(1.0f);
					ofSetColor(255, 100, 100);
					this->focusLineGraph.draw();

					ofPopStyle();
					ofPopMatrix();
				}

				//crosshair
				ofPushStyle();
				ofSetLineWidth(1);
				ofSetColor(255);
				ofPushMatrix();
				ofTranslate(args.size.x / 2.0f, args.size.y / 2.0f);
				ofLine(-10, 0, 10, 0);
				ofLine(0, -10, 0, 10);
				ofPopMatrix();
				ofPopStyle();
			};
			this->view = view;

			OFXDIGITALEMULSION_NODE_STANDARD_LISTENERS
		}

		//----------
		string Camera::getTypeName() const {
			return "Item::Camera";
		}

		//----------
		void Camera::update() {
			CHECK_GRABBER
			this->grabber->update();

			this->updateRayCamera();

			if (this->showFocusLine) {
				if (this->grabber->isFrameNew()) {
					auto pixels = this->grabber->getPixelsRef();
					auto middleRow = pixels.getPixels() + pixels.getWidth() * pixels.getNumChannels() * pixels.getHeight() / 2;

					this->focusLineGraph.clear();
					this->focusLineGraph.setMode(OF_PRIMITIVE_LINE_STRIP);
					for(int i=0; i<pixels.getWidth(); i++) {
						this->focusLineGraph.addVertex(ofVec3f(i, *middleRow, 0));
						middleRow += pixels.getNumChannels();
					}
				}
			}
		}

		//----------
		ofxCvGui::PanelPtr Camera::getView() {
			if (!this->view) {
				OFXDIGITALEMULSION_FATAL << "You must set a device on the ofxDigitalEmulsion::Camera before you can call getView";
			}
			return this->view;
		}

		//----------
		void Camera::serialize(Json::Value & json) {
			auto & jsonSettings = json["settings"];
			Utils::Serializable::serialize(this->exposure, jsonSettings);
			Utils::Serializable::serialize(this->gain, jsonSettings);
			Utils::Serializable::serialize(this->focus, jsonSettings);
			Utils::Serializable::serialize(this->sharpness, jsonSettings);

			auto & jsonCalibration = json["calibration"];
			Utils::Serializable::serialize(this->focalLengthX, jsonCalibration);
			Utils::Serializable::serialize(this->focalLengthY, jsonCalibration);
			Utils::Serializable::serialize(this->principalPointX, jsonCalibration);
			Utils::Serializable::serialize(this->principalPointY, jsonCalibration);

			auto & jsonDistortion = jsonCalibration["distortion"];
			for(int i=0; i<OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT; i++) {
				Utils::Serializable::serialize(this->distortion[i], jsonDistortion);
			}

			Utils::Serializable::serialize(this->translationX, json);
			Utils::Serializable::serialize(this->translationY, json);
			Utils::Serializable::serialize(this->translationZ, json);
			Utils::Serializable::serialize(this->rotationX, json);
			Utils::Serializable::serialize(this->rotationY, json);
			Utils::Serializable::serialize(this->rotationZ, json);

			auto & jsonResolution = json["resolution"];
			jsonResolution["width"] = this->getWidth();
			jsonResolution["height"] = this->getHeight();
		}

		//----------
		void Camera::deserialize(const Json::Value & json) {
			auto & jsonSettings = json["settings"];
			Utils::Serializable::deserialize(this->exposure, jsonSettings);
			Utils::Serializable::deserialize(this->gain, jsonSettings);
			Utils::Serializable::deserialize(this->focus, jsonSettings);
			Utils::Serializable::deserialize(this->sharpness, jsonSettings);

			auto & jsonCalibration = json["calibration"];
			Utils::Serializable::deserialize(this->focalLengthX, jsonCalibration);
			Utils::Serializable::deserialize(this->focalLengthY, jsonCalibration);
			Utils::Serializable::deserialize(this->principalPointX, jsonCalibration);
			Utils::Serializable::deserialize(this->principalPointY, jsonCalibration);

			auto & jsonDistortion = jsonCalibration["distortion"];
			for(int i=0; i<OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT; i++) {
				Utils::Serializable::deserialize(this->distortion[i], jsonDistortion);
			}

			Utils::Serializable::deserialize(this->translationX, json);
			Utils::Serializable::deserialize(this->translationY, json);
			Utils::Serializable::deserialize(this->translationZ, json);
			Utils::Serializable::deserialize(this->rotationX, json);
			Utils::Serializable::deserialize(this->rotationY, json);
			Utils::Serializable::deserialize(this->rotationZ, json);

			auto deviceOpen = false;
			if (this->grabber) {
				this->grabber->setExposure(this->exposure);
				this->grabber->setGain(this->gain);
				this->grabber->setFocus(this->focus);
				this->grabber->setSharpness(this->sharpness);
				deviceOpen = this->grabber->getIsDeviceOpen();
			}
			if (!deviceOpen) {
				auto & jsonResolution = json["resolution"];
				this->rayCamera.setWidth(jsonResolution["width"].asFloat());
				this->rayCamera.setHeight(jsonResolution["height"].asFloat());
			}
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
		float Camera::getWidth() {
			return this->rayCamera.getWidth();
		}

		//----------
		float Camera::getHeight() {
			return this->rayCamera.getHeight();
		}

		//----------
		void Camera::setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients) {
			this->focalLengthX = cameraMatrix.at<double>(0, 0);
			this->focalLengthY = cameraMatrix.at<double>(1, 1);
			this->principalPointX = cameraMatrix.at<double>(0, 2);
			this->principalPointY = cameraMatrix.at<double>(1, 2);
			for(int i=0; i<OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT; i++) {
				this->distortion[i] = distortionCoefficients.at<double>(i);
			}
		}

		//----------
		void Camera::setExtrinsics(cv::Mat rotation, cv::Mat translation) {
			const auto rotationMatrix = ofxCv::makeMatrix(rotation, cv::Mat::zeros(3, 1, CV_64F));
			const auto rotationEuler = rotationMatrix.getRotate().getEuler();

			this->translationX = (float) translation.at<double>(0);
			this->translationY = (float) translation.at<double>(1);
			this->translationZ = (float) translation.at<double>(2);

			this->rotationX = (float) rotationEuler.x;
			this->rotationY = (float) rotationEuler.y;
			this->rotationZ = (float) rotationEuler.z;
			//this->rebuildProjector();
		}

		//----------
		Mat Camera::getCameraMatrix() const {
			Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
			cameraMatrix.at<double>(0, 0) = this->focalLengthX;
			cameraMatrix.at<double>(1, 1) = this->focalLengthY;
			cameraMatrix.at<double>(0, 2) = this->principalPointX;
			cameraMatrix.at<double>(1, 2) = this->principalPointY;
			return cameraMatrix;
		}

		//----------
		Mat Camera::getDistortionCoefficients() const {
			Mat distortionCoefficients = Mat::zeros(8, 1, CV_64F);
			for(int i=0; i<OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT; i++) {
				distortionCoefficients.at<double>(i) = this->distortion[i];
			}
			return distortionCoefficients;
		}

		//----------
		const ofxRay::Camera & Camera::getRayCamera() const {
			return this->rayCamera;
		}
		
		//----------
		void Camera::drawWorld() {
			this->rayCamera.draw();
			ofDrawBitmapString(this->getName(), this->rayCamera.getPosition());
		}

		//----------
		ofPixels Camera::getFreshFrame() {
			if (!this->grabber) {
				return ofPixels();
			} else {
				return this->grabber->getFreshFrame()->getPixelsRef();
			}
		}

		//----------
		void Camera::populateInspector(ElementGroupPtr inspector) {
			inspector->add(Widgets::LiveValueHistory::make("Device fps [Hz]", [this] () {
				if (this->grabber) {
					return this->grabber->getFps();
				} else {
					return 0.0f;
				}
			}, true));
			inspector->add(Widgets::Toggle::make(this->showSpecification));
			inspector->add(Widgets::Toggle::make(this->showFocusLine));

			auto grabber = this->getGrabber();
			if (grabber) {
				if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Exposure)) {
					auto exposureSlider = Widgets::Slider::make(this->exposure);
					exposureSlider->addIntValidator();
					inspector->add(exposureSlider);
				}
				if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Gain)) {
					inspector->add(Widgets::Slider::make(this->gain));
				}
				if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Focus)) {
					inspector->add(Widgets::Slider::make(this->focus));
				}
				if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Sharpness)) {
					inspector->add(Widgets::Slider::make(this->sharpness));
				}
				if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_OneShot)) {
					inspector->add(MAKE(Widgets::Button, "Take Photo", [this]() {
						this->getGrabber()->singleShot();
					}));
				}
			}
			
			inspector->add(Widgets::Spacer::make());
			
			inspector->add(Widgets::Title::make("Camera matrix", Widgets::Title::Level::H3));
			inspector->add(Widgets::Slider::make(this->focalLengthX));
			inspector->add(Widgets::Slider::make(this->focalLengthY));
			inspector->add(Widgets::Slider::make(this->principalPointX));
			inspector->add(Widgets::Slider::make(this->principalPointY));
			inspector->add(Widgets::LiveValue<float>::make("Throw ratio X", [this] () {
				return this->rayCamera.getThrowRatio();
			}));
			
			inspector->add(Widgets::Spacer::make());

			inspector->add(Widgets::Title::make("Distortion coefficients", Widgets::Title::Level::H3));
			for(int i=0; i<OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT; i++) {
				inspector->add(Widgets::Slider::make(this->distortion[i]));
			}

			inspector->add(Widgets::Slider::make(this->translationX));
			inspector->add(Widgets::Slider::make(this->translationY));
			inspector->add(Widgets::Slider::make(this->translationZ));
			inspector->add(Widgets::Slider::make(this->rotationX));
			inspector->add(Widgets::Slider::make(this->rotationY));
			inspector->add(Widgets::Slider::make(this->rotationZ));
		}

		//----------
		void Camera::updateRayCamera() {
			if (this->grabber) {
				if (this->grabber->getWidth() != 0 && this->grabber->getHeight() != 0) {
					this->rayCamera.setWidth(this->grabber->getWidth());
					this->rayCamera.setHeight(this->grabber->getHeight());
				}
			}
			this->rayCamera.setProjection(ofxCv::makeProjectionMatrix(this->getCameraMatrix(), cv::Size(this->getWidth(), this->getHeight())));

			ofQuaternion rotation;
			auto rotationQuat = ofQuaternion(this->rotationX, ofVec3f(1, 0, 0), this->rotationZ, ofVec3f(0, 0, 1), this->rotationY, ofVec3f(0, 1, 0));
			ofMatrix4x4 pose = ofMatrix4x4(rotationQuat);
			pose(3, 0) = this->translationX;
			pose(3, 1) = this->translationY;
			pose(3, 2) = this->translationZ;
			this->rayCamera.setView(pose);
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
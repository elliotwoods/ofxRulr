#include "View.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Spacer.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/LiveValue.h"

using namespace ofxCvGui;
using namespace cv;

namespace ofxDigitalEmulsion {
	namespace Item {
		//---------
		View::View() {
			OFXDIGITALEMULSION_NODE_INIT_LISTENER;
		}

		//---------
		string View::getTypeName() const {
			return "Item::View";
		}

		//---------
		void View::init() {
			OFXDIGITALEMULSION_NODE_UPDATE_LISTENER;
			OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
			OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;
		
			this->focalLengthX.set("Focal Length X", 1024.0f, 1.0f, 50000.0f);
			this->focalLengthY.set("Focal Length Y", 1024.0f, 1.0f, 50000.0f);
			this->principalPointX.set("Center Of Projection X", 512.0f, -10000.0f, 10000.0f);
			this->principalPointY.set("Center Of Projection Y", 512.0f, -10000.0f, 10000.0f);
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				this->distortion[i].set("Distortion K" + ofToString(i + 1), 0.0f, -1000.0f, 1000.0f);
			}
		}

		//---------
		void View::update() {
			this->rayCameraObject.setProjection(ofxCv::makeProjectionMatrix(this->getCameraMatrix(), cv::Size(this->getWidth(), this->getHeight())));
			this->rayCameraWorld = this->rayCameraObject;
			this->rayCameraWorld.setView(RigidBody::getTransform());
		}

		//---------
		void View::serialize(Json::Value & json) {
			auto & jsonCalibration = json["calibration"];
			Utils::Serializable::serialize(this->focalLengthX, jsonCalibration);
			Utils::Serializable::serialize(this->focalLengthY, jsonCalibration);
			Utils::Serializable::serialize(this->principalPointX, jsonCalibration);
			Utils::Serializable::serialize(this->principalPointY, jsonCalibration);

			auto & jsonDistortion = jsonCalibration["distortion"];
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				Utils::Serializable::serialize(this->distortion[i], jsonDistortion);
			}
		}

		//---------
		void View::deserialize(const Json::Value & json) {
			auto & jsonCalibration = json["calibration"];
			Utils::Serializable::deserialize(this->focalLengthX, jsonCalibration);
			Utils::Serializable::deserialize(this->focalLengthY, jsonCalibration);
			Utils::Serializable::deserialize(this->principalPointX, jsonCalibration);
			Utils::Serializable::deserialize(this->principalPointY, jsonCalibration);

			auto & jsonDistortion = jsonCalibration["distortion"];
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				Utils::Serializable::deserialize(this->distortion[i], jsonDistortion);
			}
		}
		
		//---------
		void View::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make("Camera matrix", Widgets::Title::Level::H3));
			inspector->add(Widgets::Slider::make(this->focalLengthX));
			inspector->add(Widgets::Slider::make(this->focalLengthY));
			inspector->add(Widgets::Slider::make(this->principalPointX));
			inspector->add(Widgets::Slider::make(this->principalPointY));
			inspector->add(Widgets::LiveValue<float>::make("Throw ratio X", [this]() {
				return this->rayCameraObject.getThrowRatio();
			}));
			inspector->add(Widgets::LiveValue<float>::make("Aspect ratio", [this]() {
				return this->focalLengthY / this->focalLengthX;
			}));

			inspector->add(Widgets::Spacer::make());

			inspector->add(Widgets::Title::make("Distortion coefficients", Widgets::Title::Level::H3));
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				inspector->add(Widgets::Slider::make(this->distortion[i]));
			}

			inspector->add(make_shared<Widgets::Spacer>());
		}

		//----------
		float View::getWidth() const {
			return this->rayCameraObject.getWidth();
		}

		//----------
		float View::getHeight() const {
			return this->rayCameraObject.getHeight();
		}

		//----------
		void View::setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients) {
			this->focalLengthX = cameraMatrix.at<double>(0, 0);
			this->focalLengthY = cameraMatrix.at<double>(1, 1);
			this->principalPointX = cameraMatrix.at<double>(0, 2);
			this->principalPointY = cameraMatrix.at<double>(1, 2);
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				this->distortion[i] = distortionCoefficients.at<double>(i);
			}
		}

		//----------
		Mat View::getCameraMatrix() const {
			Mat cameraMatrix = Mat::eye(3, 3, CV_64F);
			cameraMatrix.at<double>(0, 0) = this->focalLengthX;
			cameraMatrix.at<double>(1, 1) = this->focalLengthY;
			cameraMatrix.at<double>(0, 2) = this->principalPointX;
			cameraMatrix.at<double>(1, 2) = this->principalPointY;
			return cameraMatrix;
		}

		//----------
		Mat View::getDistortionCoefficients() const {
			Mat distortionCoefficients = Mat::zeros(8, 1, CV_64F);
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				distortionCoefficients.at<double>(i) = this->distortion[i];
			}
			return distortionCoefficients;
		}

		//----------
		const ofxRay::Camera & View::getRayCameraWorld() const {
			return this->rayCameraWorld;
		}

		//----------
		const ofxRay::Camera & View::getRayCameraObject() const {
			return this->rayCameraObject;
		}

		//----------
		void View::drawObject() {
			this->rayCameraObject.draw();
			ofDrawBitmapString(this->getName(), ofVec3f());
		}
	}
}
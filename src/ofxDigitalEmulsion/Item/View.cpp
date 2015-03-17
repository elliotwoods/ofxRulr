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
		
			this->onTransformChange += [this]() {
				this->rebuildViewFromParameters();
			};

			this->focalLengthX.set("Focal Length X", 1024.0f, 1.0f, 50000.0f);
			this->focalLengthY.set("Focal Length Y", 1024.0f, 1.0f, 50000.0f);
			this->principalPointX.set("Center Of Projection X", 512.0f, -10000.0f, 10000.0f);
			this->principalPointY.set("Center Of Projection Y", 512.0f, -10000.0f, 10000.0f);
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				this->distortion[i].set("Distortion K" + ofToString(i + 1), 0.0f, -1000.0f, 1000.0f);
			}

			this->focalLengthX.addListener(this, &View::parameterCallback);
			this->focalLengthY.addListener(this, &View::parameterCallback);
			this->principalPointX.addListener(this, &View::parameterCallback);
			this->principalPointX.addListener(this, &View::parameterCallback);

			this->viewInObjectSpace.setDefaultFar(20.0f);
		}

		//---------
		void View::update() {

		}

		//----------
		void View::drawObject() {
			this->viewInObjectSpace.draw();
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
			inspector->add(Widgets::Title::make("View", Widgets::Title::Level::H2));
			inspector->add(make_shared<Widgets::LiveValue<string>>("Resolution", [this](){
				string msg;
				msg = ofToString((int)this->getWidth()) + ", " + ofToString((int)this->getHeight());
				return msg;
			}));

			inspector->add(Widgets::Title::make("Camera matrix", Widgets::Title::Level::H3));
			inspector->add(Widgets::Slider::make(this->focalLengthX));
			inspector->add(Widgets::Slider::make(this->focalLengthY));
			inspector->add(Widgets::Slider::make(this->principalPointX));
			inspector->add(Widgets::Slider::make(this->principalPointY));
			inspector->add(Widgets::LiveValue<float>::make("Throw ratio X", [this]() {
				return this->viewInObjectSpace.getThrowRatio();
			}));
			inspector->add(Widgets::LiveValue<float>::make("Aspect ratio", [this]() {
				return this->focalLengthY / this->focalLengthX;
			}));
			inspector->add(Widgets::LiveValue<ofVec2f>::make("Lens offset", [this]() {
				return this->getViewInObjectSpace().getLensOffset();
			}));

			inspector->add(Widgets::Spacer::make());

			inspector->add(Widgets::Title::make("Distortion coefficients", Widgets::Title::Level::H3));
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				inspector->add(Widgets::Slider::make(this->distortion[i]));
			}

			inspector->add(Widgets::Button::make("Export View matrix...", [this]() {
				try {
					this->exportViewMatrix();
				}
				OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
			}));


			inspector->add(make_shared<Widgets::Spacer>());
		}

		//----------
		float View::getWidth() const {
			return this->viewInObjectSpace.getWidth();
		}

		//----------
		float View::getHeight() const {
			return this->viewInObjectSpace.getHeight();
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
			this->rebuildViewFromParameters();
		}

		//----------
		void View::setProjection(const ofMatrix4x4 & projection) {
			ofLogWarning("View::setProjection") << "Calls to this function will only change cached objects (not parameters). Use this function for debug purposes only.";
			this->viewInObjectSpace.setProjection(projection);
		}

		//----------
		cv::Size View::getSize() const {
			return cv::Size(this->getWidth(), this->getHeight());
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
			Mat distortionCoefficients = Mat::zeros(OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT, 1, CV_64F);
			for (int i = 0; i<OFXDIGITALEMULSION_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
				distortionCoefficients.at<double>(i) = this->distortion[i];
			}
			return distortionCoefficients;
		}

		//----------
		const ofxRay::Camera & View::getViewInObjectSpace() const {
			return this->viewInObjectSpace;
		}

		//----------
		ofxRay::Camera View::getViewInWorldSpace() const {
			auto viewInWorldSpace = this->viewInObjectSpace;

			const auto viewInverse = this->getTransform();
			viewInWorldSpace.setView(viewInverse.getInverse());

			return viewInWorldSpace;
		}

		//----------
		void View::rebuildViewFromParameters() {
			auto projection = ofxCv::makeProjectionMatrix(this->getCameraMatrix(), this->getSize());
			this->viewInObjectSpace.setProjection(projection);
		}

		//----------
		void View::exportViewMatrix() {
			const auto matrix = this->getViewInObjectSpace().getClippedProjectionMatrix();
			auto result = ofSystemSaveDialog(this->getName() + "-Projection.mat", "Export View matrix");
			if (result.bSuccess) {
				ofstream fileout(ofToDataPath(result.filePath), ios::binary | ios::out);
				fileout.write((char*)& matrix, sizeof(matrix));
				fileout.close();
			}
		}

		//---------
		void View::parameterCallback(float &) {
			this->rebuildViewFromParameters();
		}
	}
}
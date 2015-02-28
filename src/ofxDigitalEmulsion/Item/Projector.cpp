#include "Projector.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Projector::Projector() {
			OFXDIGITALEMULSION_NODE_INIT_LISTENER;
		}

		//----------
		void Projector::init() {
			OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
			OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;

			this->projector.setDefaultNear(0.05f);
			this->projector.setDefaultFar(2.0f);

			this->resolutionWidth.set("Resolution width", 1024.0f, 1.0f, 8 * 1280.0f);
			this->resolutionHeight.set("Resolution height", 768.0f, 1.0f, 8 * 800.0f);
			this->throwRatioX.set("Throw ratio X", 1.4f, 0.01f, 10.0f);
			this->pixelAspectRatio.set("Pixel aspect ratio", 1.0f, 0.01f, 10.0f);
			this->lensOffsetX.set("Lens offset X", 0.0f, -2.0f, 2.0f);
			this->lensOffsetY.set("Lens offset Y", 0.5f, -2.0f, 2.0f);
			this->translationX.set("Translation X", 0.0f, -100.0f, 100.0f);
			this->translationY.set("Translation Y", 0.0f, -100.0f, 100.0f);
			this->translationZ.set("Translation Z", 0.0f, -100.0f, 100.0f);
			this->rotationX.set("Rotation X", 0.0f, -360.0f, 360.0f);
			this->rotationY.set("Rotation Y", 0.0f, -360.0f, 360.0f);
			this->rotationZ.set("Rotation Z", 0.0f, -360.0f, 360.0f);

			this->resolutionWidth.addListener(this, &Projector::projectorParameterCallback);
			this->resolutionHeight.addListener(this, &Projector::projectorParameterCallback);
			this->throwRatioX.addListener(this, &Projector::projectorParameterCallback);
			this->pixelAspectRatio.addListener(this, &Projector::projectorParameterCallback);
			this->lensOffsetX.addListener(this, &Projector::projectorParameterCallback);
			this->lensOffsetY.addListener(this, &Projector::projectorParameterCallback);
			this->translationX.addListener(this, &Projector::projectorParameterCallback);
			this->translationY.addListener(this, &Projector::projectorParameterCallback);
			this->translationZ.addListener(this, &Projector::projectorParameterCallback);
			this->rotationX.addListener(this, &Projector::projectorParameterCallback);
			this->rotationY.addListener(this, &Projector::projectorParameterCallback);
			this->rotationZ.addListener(this, &Projector::projectorParameterCallback);

			this->rebuildProjector();
		}

		//----------
		string Projector::getTypeName() const {
			return "Item::Projector";
		}
			
		//----------
		void Projector::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->resolutionWidth, json);
			Utils::Serializable::serialize(this->resolutionHeight, json);
			Utils::Serializable::serialize(this->throwRatioX, json);
			Utils::Serializable::serialize(this->pixelAspectRatio, json);
			Utils::Serializable::serialize(this->lensOffsetX, json);
			Utils::Serializable::serialize(this->lensOffsetY, json);

			Utils::Serializable::serialize(this->translationX, json);
			Utils::Serializable::serialize(this->translationY, json);
			Utils::Serializable::serialize(this->translationZ, json);
			Utils::Serializable::serialize(this->rotationX, json);
			Utils::Serializable::serialize(this->rotationY, json);
			Utils::Serializable::serialize(this->rotationZ, json);
		}

		//----------
		void Projector::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->resolutionWidth, json);
			Utils::Serializable::deserialize(this->resolutionHeight, json);
			Utils::Serializable::deserialize(this->throwRatioX, json);
			Utils::Serializable::deserialize(this->pixelAspectRatio, json);
			Utils::Serializable::deserialize(this->lensOffsetX, json);
			Utils::Serializable::deserialize(this->lensOffsetY, json);

			Utils::Serializable::deserialize(this->translationX, json);
			Utils::Serializable::deserialize(this->translationY, json);
			Utils::Serializable::deserialize(this->translationZ, json);
			Utils::Serializable::deserialize(this->rotationX, json);
			Utils::Serializable::deserialize(this->rotationY, json);
			Utils::Serializable::deserialize(this->rotationZ, json);

			this->rebuildProjector();
		}

		//----------
		float Projector::getWidth() const {
			return this->resolutionWidth;
		}

		//----------
		float Projector::getHeight() const {
			return this->resolutionHeight;
		}

		//----------
		void Projector::setWidth(float width) {
			this->resolutionWidth = width;
		}

		//----------
		void Projector::setHeight(float height) {
			this->resolutionWidth = height;
		}

		//----------
		void Projector::setIntrinsics(cv::Mat cameraMatrix) {
			float fovx = cameraMatrix.at<double>(0, 0);
			float fovy = cameraMatrix.at<double>(1, 1);
			float ppx = cameraMatrix.at<double>(0, 2);
			float ppy = cameraMatrix.at<double>(1, 2);

			this->throwRatioX = fovx / this->getWidth();
			auto throwRatioY = fovy / this->getHeight();
			auto throwAspectRatio = this->throwRatioX / throwRatioY;
			this->pixelAspectRatio = throwAspectRatio / this->getResolutionAspectRatio();
			
			this->lensOffsetX = (ppx / this->getWidth()) - 0.5f; // not sure if this is + or -ve (if wrong, then both this and ofxCvMin::Helpers::makeProjectionMatrix should be switched
			this->lensOffsetY = (ppy / this->getHeight()) - 0.5f;
			//this may be out by a factor of 2. i.e.
			//this->lensOffsetX /= 2.0f;
			//this->lensOffsetY /= 2.0f;

			const auto newProjection = ofxCv::makeProjectionMatrix(cameraMatrix, cv::Size(projector.getWidth(), projector.getHeight()));
			this->projector.setProjection(newProjection);
			//this->rebuildProjector();
		}
		
		//----------
		void Projector::setExtrinsics(cv::Mat rotation, cv::Mat translation) {
			const auto rotationMatrix = ofxCv::makeMatrix(rotation, cv::Mat::zeros(3, 1, CV_64F));
			const auto rotationEuler = rotationMatrix.getRotate().getEuler();

			this->translationX = translation.at<double>(0);
			this->translationY = translation.at<double>(1);
			this->translationZ = translation.at<double>(2);

			this->rotationX = rotationEuler.x;
			this->rotationY = rotationEuler.y;
			this->rotationZ = rotationEuler.z;

			this->projector.setView(ofxCv::makeMatrix(rotation, translation));
			//this->rebuildProjector();
		}

		//----------
		const ofxRay::Projector & Projector::getRayProjector() const {
			return this->projector;
		}

		//----------
		void Projector::drawWorld() {
			this->projector.draw();
			ofDrawBitmapString(this->getName(), this->projector.getPosition());
		}
			
		//----------
		float Projector::getResolutionAspectRatio() const {
			return this->getWidth() / this->getHeight();
		}

		//----------
		float Projector::getPixelAspectRatio() const {
			return this->pixelAspectRatio;
		}

		//----------
		float Projector::getThrowRatioX() const {
			return this->throwRatioX;
		}

		//----------
		float Projector::getThrowRatioY() const {
			return this->throwRatioX * (this->getResolutionAspectRatio() / this->getPixelAspectRatio());
		}

		//----------
		void Projector::populateInspector(ElementGroupPtr inspector) {
			inspector->add(Widgets::Slider::make(this->resolutionWidth));
			inspector->add(Widgets::Slider::make(this->resolutionHeight));

			inspector->add(Widgets::Spacer::make());

			inspector->add(Widgets::Slider::make(this->throwRatioX));
			inspector->add(Widgets::LiveValue<float>::make("Throw ratio Y", [this]() {
				return this->getThrowRatioY();
			}));
			inspector->add(Widgets::Slider::make(this->pixelAspectRatio));
			inspector->add(Widgets::Slider::make(this->lensOffsetX));
			inspector->add(Widgets::Slider::make(this->lensOffsetY));

			inspector->add(Widgets::Spacer::make());

			inspector->add(Widgets::Slider::make(this->translationX));
			inspector->add(Widgets::Slider::make(this->translationY));
			inspector->add(Widgets::Slider::make(this->translationZ));
			inspector->add(Widgets::Slider::make(this->rotationX));
			inspector->add(Widgets::Slider::make(this->rotationY));
			inspector->add(Widgets::Slider::make(this->rotationZ));
		}

		//----------
		void Projector::rebuildProjector() {
			ofQuaternion rotation;
			auto rotationQuat = ofQuaternion(this->rotationX, ofVec3f(1, 0, 0), this->rotationZ, ofVec3f(0, 0, 1), this->rotationY, ofVec3f(0, 1, 0));
			ofMatrix4x4 pose = ofMatrix4x4(rotationQuat);
			pose(3,0) = this->translationX;
			pose(3,1) = this->translationY;
			pose(3,2) = this->translationZ;
			this->projector.setView(pose);

			ofMatrix4x4 projection;
			projection(0,0) = this->getThrowRatioX();
			projection(1,1) = -this->getThrowRatioY();
			projection(2,3) = 1.0f;
			projection(3,3) = 0.0f;
			projection.postMultTranslate(-this->lensOffsetX * 2.0f, -this->lensOffsetY * 2.0f, 0.0f);
			this->projector.setProjection(projection);

			this->projector.setWidth(this->getWidth());
			this->projector.setHeight(this->getHeight());
		}

		//----------
		void Projector::projectorParameterCallback(float &) {
			rebuildProjector();
		}
	}
}
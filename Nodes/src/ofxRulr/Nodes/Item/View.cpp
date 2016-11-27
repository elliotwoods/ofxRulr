#include "pch_RulrNodes.h"
#include "View.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Spacer.h"
#include "ofxCvGui/Widgets/Slider.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/EditableValue.h"

using namespace ofxCvGui;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//---------
			View::View(bool hasDistortion) : hasDistortion(hasDistortion) {
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string View::getTypeName() const {
				return "Item::View";
			}

			//---------
			void View::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				this->onTransformChange += [this]() {
					this->markViewDirty();
				};

				this->setWidth(1024);
				this->setHeight(768);
				this->focalLengthX.set("Focal Length X", this->getWidth(), 1.0f, 50000.0f);
				this->focalLengthY.set("Focal Length Y", this->getWidth(), 1.0f, 50000.0f);
				this->principalPointX.set("Center Of Projection X", this->getWidth() / 2.0f, -10000.0f, 10000.0f);
				this->principalPointY.set("Center Of Projection Y", this->getHeight() / 2.0f, -10000.0f, 10000.0f);
				for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
					this->distortion[i].set("Distortion K" + ofToString(i + 1), 0.0f, -1000.0f, 1000.0f);
				}

				this->focalLengthX.addListener(this, &View::parameterCallback);
				this->focalLengthY.addListener(this, &View::parameterCallback);
				this->principalPointX.addListener(this, &View::parameterCallback);
				this->principalPointX.addListener(this, &View::parameterCallback);
				for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
					this->distortion[i].addListener(this, &View::parameterCallback);
				}
				this->parameters.clipping._near.addListener(this, &View::parameterCallback);
				this->parameters.clipping._far.addListener(this, &View::parameterCallback);

				this->viewInObjectSpaceCached.color = this->getColor();

				this->markViewDirty();
			}

			//---------
			void View::update() {

			}

			//----------
			void View::drawObject() {
				this->getViewInObjectSpace().draw();
			}

			//---------
			void View::serialize(Json::Value & json) {
				auto & jsonCalibration = json["calibration"];
				Utils::Serializable::serialize(jsonCalibration, this->focalLengthX);
				Utils::Serializable::serialize(jsonCalibration, this->focalLengthY);
				Utils::Serializable::serialize(jsonCalibration, this->principalPointX);
				Utils::Serializable::serialize(jsonCalibration, this->principalPointY);

				auto & jsonResolution = json["resolution"];
				{
					auto viewInObjectSpace = this->getViewInObjectSpace();
					jsonResolution["width"] = viewInObjectSpace.getWidth();
					jsonResolution["height"] = viewInObjectSpace.getHeight();
				}

				auto & jsonDistortion = jsonCalibration["distortion"];
				for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
					Utils::Serializable::serialize(jsonDistortion, this->distortion[i]);
				}
			}

			//---------
			void View::deserialize(const Json::Value & json) {
				const auto & jsonCalibration = json["calibration"];
				Utils::Serializable::deserialize(jsonCalibration, this->focalLengthX);
				Utils::Serializable::deserialize(jsonCalibration, this->focalLengthY);
				Utils::Serializable::deserialize(jsonCalibration, this->principalPointX);
				Utils::Serializable::deserialize(jsonCalibration, this->principalPointY);

				const auto & jsonResolution = json["resolution"];
				if (!jsonResolution.isNull()) {
					this->setWidth(jsonResolution["width"].asFloat());
					this->setHeight(jsonResolution["height"].asFloat());
				}

				auto & jsonDistortion = jsonCalibration["distortion"];
				for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
					Utils::Serializable::deserialize(jsonDistortion, this->distortion[i]);
				}

				this->markViewDirty();
			}

			//---------
			void View::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Title("View", Widgets::Title::Level::H2));
				inspector->add(make_shared<Widgets::LiveValue<string>>("Resolution", [this](){
					string msg;
					msg = ofToString((int)this->getWidth()) + ", " + ofToString((int)this->getHeight());
					return msg;
				}));

				inspector->add(new Widgets::Title("Camera matrix", Widgets::Title::Level::H3));
				auto addCameraMatrixParameter = [this, inspector](ofParameter<float> & parameter) {
					auto slider = new Widgets::Slider(parameter);
					slider->onValueChange += [this](ofParameter<float> &) {
						this->markViewDirty();
					};
					inspector->add(slider);
				};

				addCameraMatrixParameter(this->focalLengthX);
				addCameraMatrixParameter(this->focalLengthY);
				addCameraMatrixParameter(this->principalPointX);
				addCameraMatrixParameter(this->principalPointY);

				inspector->add(new Widgets::EditableValue<float>("Throw ratio X", [this]() {
					return this->getThrowRatio();
				}, [this](string newValueString) {
					auto newThrowRatio = ofToFloat(newValueString);
					if (newThrowRatio > 0.0f) {
						this->setThrowRatio(newThrowRatio);
					}
				}));
				inspector->add(new Widgets::EditableValue<float>("Pixel aspect ratio", [this]() {
					return this->getPixelAspectRatio();
				}, [this](string newValueString) {
					auto newPixelAspectRatio = ofToFloat(newValueString);
					if (newPixelAspectRatio > 0.0f) {
						this->setPixelAspectRatio(newPixelAspectRatio);
					}
				}));

				inspector->add(new Widgets::EditableValue<ofVec2f>("Lens offset", [this]() {
					return this->getLensOffset();
				}, [this](string newValueString) {
					auto newValueStrings = ofSplitString(newValueString, ",");
					if (newValueStrings.size() == 2) {
						this->setLensOffset(ofVec2f(ofToFloat(newValueStrings[0]), ofToFloat(newValueStrings[1])));
					}
				}));

				if (this->hasDistortion) {
					inspector->add(new Widgets::Spacer());

					inspector->add(new Widgets::Title("Distortion coefficients", Widgets::Title::Level::H3));
					for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
						inspector->addSlider(this->distortion[i])->onValueChange += [this](ofParameter<float>&) {
							this->markViewDirty();
						};
					}
				}

				inspector->add(new Widgets::Spacer());

				inspector->add(new Widgets::Button("Export View matrix...", [this]() {
					try {
						this->exportViewMatrix();
					}
					RULR_CATCH_ALL_TO_ALERT
				}));

				inspector->add(new Widgets::Button("Export ofxRay::Camera...", [this]() {
					try {
						this->exportRayCamera();
					}
					RULR_CATCH_ALL_TO_ALERT
				}));

				inspector->add(new Widgets::Button("Export YML...", [this]() {
					try {
						this->exportYaml();
					}
					RULR_CATCH_ALL_TO_ALERT
				}));

				inspector->add(make_shared<Widgets::Spacer>());
			}

			//----------
			void View::setWidth(float width) {
				this->viewInObjectSpaceCached.setWidth(width);
				this->markViewDirty();
			}

			//----------
			void View::setHeight(float height) {
				this->viewInObjectSpaceCached.setHeight(height);
				this->markViewDirty();
			}

			//----------
			float View::getWidth() const {
				return this->viewInObjectSpaceCached.getWidth();
			}

			//----------
			float View::getHeight() const {
				return this->viewInObjectSpaceCached.getHeight();
			}

			//----------
			void View::setIntrinsics(cv::Mat cameraMatrix, cv::Mat distortionCoefficients) {
				this->focalLengthX = cameraMatrix.at<double>(0, 0);
				this->focalLengthY = cameraMatrix.at<double>(1, 1);
				this->principalPointX = cameraMatrix.at<double>(0, 2);
				this->principalPointY = cameraMatrix.at<double>(1, 2);
				for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
					this->distortion[i] = distortionCoefficients.at<double>(i);
				}
				this->markViewDirty();
			}
// 
// 			//----------
// 			void View::setProjection(const ofMatrix4x4 & projection) {
// 				ofLogWarning("View::setProjection") << "Calls to this function will only change cached objects (not parameters). Use this function for debug purposes only.";
// 				this->viewInObjectSpace.setProjection(projection);
// 			}

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
				Mat distortionCoefficients = Mat::zeros(RULR_VIEW_DISTORTION_COEFFICIENT_COUNT, 1, CV_64F);
				for (int i = 0; i<RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
					distortionCoefficients.at<double>(i) = this->distortion[i];
				}
				return distortionCoefficients;
			}

			//----------
			float View::getThrowRatio() const {
				return this->getViewInObjectSpace().getThrowRatio();
			}

			//----------
			void View::setThrowRatio(float throwRatio) {
				auto pixelAspectRatio = this->focalLengthY / this->focalLengthX;
				this->focalLengthX = this->getWidth() * throwRatio;
				this->focalLengthY = this->focalLengthX / pixelAspectRatio;
				this->markViewDirty();
			}

			//----------
			float View::getPixelAspectRatio() const {
				return this->focalLengthY / this->focalLengthX;
			}

			//----------
			void View::setPixelAspectRatio(float pixelAspectRatio) {
				this->focalLengthY = this->focalLengthX / pixelAspectRatio;
				this->markViewDirty();
			}

			//----------
			ofVec2f View::getLensOffset() const {
				return this->getViewInObjectSpace().getLensOffset();
			}

			//----------
			void View::setLensOffset(const ofVec2f & lensOffset) {
				this->principalPointX = ofMap(lensOffset.x, +0.5f, -0.5f, 0, this->getWidth());
				this->principalPointY = ofMap(lensOffset.y, -0.5f, +0.5f, 0, this->getHeight());
				this->markViewDirty();
			}

			//----------
			const ofxRay::Camera & View::getViewInObjectSpace() const {
				if (this->viewIsDirty) {
					const_cast<View *>(this)->rebuildView();
				}
				return this->viewInObjectSpaceCached;
			}

			//----------
			ofxRay::Camera View::getViewInWorldSpace() const {
				auto viewInWorldSpace = this->getViewInObjectSpace();

				const auto viewInverse = this->getTransform();
				viewInWorldSpace.setView(viewInverse.getInverse());

				return viewInWorldSpace;
			}

			//----------
			void View::markViewDirty() {
				this->viewIsDirty = true;
			}

			//----------
			void View::exportViewMatrix() {
				const auto matrix = this->getViewInObjectSpace().getClippedProjectionMatrix();
				auto result = ofSystemSaveDialog(this->getName() + "-Projection.mat", "Export View matrix");
				if (result.bSuccess) {
					ofstream fileout(ofToDataPath(result.filePath), ios::out);
					fileout.write((char*)& matrix, sizeof(matrix));
					fileout.close();
				}
			}

			//----------
			void View::exportRayCamera() {
				const auto view = this->getViewInWorldSpace();
				auto result = ofSystemSaveDialog(this->getName() + ".ofxRayCamera", "Export ofxRay::Camera");
				if (result.bSuccess) {
					ofstream fileout(ofToDataPath(result.filePath), ios::out);
					fileout << view;
					fileout.close();
				}
			}

			//----------
			void View::exportYaml() {
				auto result = ofSystemSaveDialog(this->getName() + ".yml", "Export Camera Calibration YML");
				if (result.bSuccess) {
					//adapted from https://github.com/Itseez/opencv/blob/master/samples/cpp/calibration.cpp#L170
					cv::FileStorage fs(result.filePath, cv::FileStorage::WRITE);

					fs << "image_width" << this->getWidth();
					fs << "image_height" << this->getHeight();

					fs << "camera_matrix" << this->getCameraMatrix();
					fs << "distortion_coefficients" << this->getDistortionCoefficients();
				}
			}

			//----------
			void View::rebuildView() {
				auto projection = ofxCv::makeProjectionMatrix(this->getCameraMatrix(), this->getSize());
				this->viewInObjectSpaceCached.setNearClip(this->parameters.clipping._near);
				this->viewInObjectSpaceCached.setFarClip(this->parameters.clipping._far);

				this->viewInObjectSpaceCached.setProjection(projection);

				if (this->hasDistortion) {
					auto distortionVector = vector<float>(RULR_VIEW_DISTORTION_COEFFICIENT_COUNT);
					for (int i = 0; i < RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
						distortionVector[i] = this->distortion[i].get();
					}
					this->viewInObjectSpaceCached.distortion = distortionVector;
				}

				this->viewIsDirty = false;
			}

			//---------
			void View::parameterCallback(float &) {
				this->markViewDirty();
			}
		}
	}
}
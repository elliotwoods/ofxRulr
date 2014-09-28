#include "HomographyFromGraycode.h"

#include "../../Utils/Exception.h"

#include "../Scan/Graycode.h"
#include "../../Item/Camera.h"
#include "ofxCvMin.h"

#include "ofxCvGui/Panels/Image.h"
#include "ofxCvGui/Widgets/Button.h"

#include "ofxNonLinearFit.h"

using namespace ofxCvGui;
using namespace ofxCv;

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//----------
			HomographyFromGraycode::HomographyFromGraycode() {
				this->grid = ofMesh::plane(1.0f, 1.0f, 11.0f, 11.0f);
				for (auto & vertex : grid.getVertices()) {
					vertex += ofVec3f(0.5f, 0.5f, 0.0f);
				}
				this->inputPins.push_back(MAKE(Graph::Pin<Scan::Graycode>));
				auto view = MAKE(ofxCvGui::Panels::Image, this->dummy);
				view->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
					try {
						auto graycodeNode = this->getInput<Scan::Graycode>();
						if (graycodeNode) {
							auto & dataSet = graycodeNode->getDataSet();
							if (dataSet.getHasData()) {
								ofPushMatrix();
								graycodeNode->getDecoder().draw(0, 0);

								ofMultMatrix(this->cameraToProjector.getInverse());
								ofScale(dataSet.getPayloadWidth(), dataSet.getPayloadHeight());
								ofPushStyle();
								ofNoFill();
								ofSetLineWidth(1.0f);
								this->grid.drawWireframe();
								ofPopStyle();
								ofPopMatrix();
							}
						}
					}
					catch (...) {

					}
				};
				this->view = view;
			}

			//----------
			string HomographyFromGraycode::getTypeName() const {
				return "HomographyFromGraycode";
			}

			//----------
			Graph::PinSet HomographyFromGraycode::getInputPins() const {
				return this->inputPins;
			}

			//----------
			ofxCvGui::PanelPtr HomographyFromGraycode::getView() {
				return this->view;
			}

			//----------
			void HomographyFromGraycode::serialize(Json::Value & json) {
				auto & jsonCameraToProjector = json["cameraToProjector"];
				for (int j = 0; j < 4; j++) {
					auto & jsonCameraToProjectorRow = jsonCameraToProjector[j];
					for (int i = 0; i < 4; i++) {
						jsonCameraToProjectorRow[i] = this->cameraToProjector(i, j);
					}
				}
			}

			//----------
			void HomographyFromGraycode::deserialize(const Json::Value & json) {
				const auto & jsonCameraToProjector = json["cameraToProjector"];
				for (int j = 0; j < 4; j++) {
					const auto & jsonCameraToProjectorRow = jsonCameraToProjector[j];
					for (int i = 0; i < 4; i++) {
						this->cameraToProjector(i, j) = jsonCameraToProjectorRow[i].asFloat();
					}
				}
			}

			//----------
			void HomographyFromGraycode::update() {
				auto graycodeNode = this->getInput<Scan::Graycode>();
				if (graycodeNode) {
					this->view->setImage(graycodeNode->getDecoder().getProjectorInCamera());
				}
			}

			//----------
			void HomographyFromGraycode::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {
				auto findHomographyButton = MAKE(ofxCvGui::Widgets::Button, "Find Homography", [this]() {
					try {
						this->findHomography();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				findHomographyButton->setHeight(100.0f);
				inspector->add(findHomographyButton);

				inspector->add(MAKE(ofxCvGui::Widgets::Button, "Export mapping image...", [this]() {
					try {
						this->saveMappingImage();
					}
					OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT
				}));
			}

			//----------
			void HomographyFromGraycode::findHomography() {
				this->throwIfMissingAnyConnection();

				auto graycodeNode = this->getInput<Scan::Graycode>();
				auto & dataSet = graycodeNode->getDataSet();
				if (!dataSet.getHasData()) {
					throw(ofxDigitalEmulsion::Utils::Exception("No data loaded for [ofxGraycode::DataSet]"));
				}

				vector<ofVec2f> camera;
				vector<ofVec2f> projector;

				for (auto & pixel : dataSet) {
					if (pixel.active) {
						camera.push_back(pixel.getCameraXY());
						projector.push_back(pixel.getProjectorXY());
					}
				}

				auto result = cv::findHomography(ofxCv::toCv(camera), ofxCv::toCv(projector), CV_LMEDS, 5.0);

				this->cameraToProjector.set(
					result.at<double>(0, 0), result.at<double>(1, 0), 0.0, result.at<double>(2, 0),
					result.at<double>(0, 1), result.at<double>(1, 1), 0.0, result.at<double>(2, 1),
					0.0, 0.0, 1.0, 0.0,
					result.at<double>(0, 2), result.at<double>(1, 2), 0.0, result.at<double>(2, 2));
			}

			//----------
			void HomographyFromGraycode::findDistortionCoefficients() {
				struct DataPoint {
				};

				class Model : public ofxNonLinearFit::Models::Base<DataPoint, Model> {
				public:
					cv::Mat distortionCoefficients;
					cv::Mat homography;

					const vector<ofVec2f> & cameraDistorted;
					vector<ofVec2f> cameraUndistorted;
					const vector<ofVec2f> & projector;

					Model(const vector<ofVec2f> & cameraDistorted, const vector<ofVec2f> & projector) :
					cameraDistorted(cameraDistorted), projector(projector) {

					}

					unsigned int getParameterCount() const override {
						return OFXDIGITALEMULSION_CAMERA_DISTORTION_COEFFICIENT_COUNT;
					}

					double getResidual(DataPoint point) const override {
						return 0.0f;
					}

					void evaluate(DataPoint &) const override {
						//do nothing
					}

					void cacheModel() {
						/*which camera matrix?
						cv::undistortPoints(toCv(this->cameraDistorted), toCv(this->cameraUndistorted), )
						undistort the points
						fit a homography
						*/
					}
				};
			}

			//----------
			void HomographyFromGraycode::saveMappingImage() const {
				this->throwIfMissingAnyConnection();

				auto graycodeNode = this->getInput<Scan::Graycode>();
				auto & dataSet = graycodeNode->getDataSet();
				if (!dataSet.getHasData()) {
					throw(ofxDigitalEmulsion::Utils::Exception("No data loaded for [ofxGraycode::DataSet]"));
				}

				if (this->cameraToProjector.isIdentity()) {
					throw(ofxDigitalEmulsion::Utils::Exception("No mapping has been found yet, so can't save"));
				}

				auto result = ofSystemSaveDialog("cameraToProjector.exr", "Save mapping image");
				if (!result.bSuccess) {
					return;
				}

				auto mappingGrid = this->grid;
				mappingGrid.clearColors();
				for (auto vertex : mappingGrid.getVertices()) {
					ofFloatColor color;
					(ofVec3f&)color = vertex;
					color.a = 1.0f;
					mappingGrid.addColor(color);
				}

				ofFbo mappingImage;
				mappingImage.allocate(dataSet.getPayloadWidth(), dataSet.getPayloadHeight(), GL_RGBA32F);
				mappingImage.begin();
				ofClear(0, 0);
				
				ofMultMatrix(this->cameraToProjector);
				ofScale(dataSet.getWidth(), dataSet.getHeight());
				ofPushStyle();
				mappingGrid.drawFaces();
				ofPopStyle();
				ofPopMatrix();

				mappingImage.end();

				ofFloatPixels pixels;
				pixels.allocate(dataSet.getPayloadWidth(), dataSet.getPayloadHeight(), OF_IMAGE_COLOR_ALPHA);
				mappingImage.readToPixels(pixels);

				ofSaveImage(pixels, result.filePath);
			}
		}
	}
}

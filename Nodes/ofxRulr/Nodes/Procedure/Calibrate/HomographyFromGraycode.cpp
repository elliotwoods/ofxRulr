#include "HomographyFromGraycode.h"

#include "ofxRulr/Exception.h"

#include "../Scan/Graycode.h"
#include "../../Item/Camera.h"
#include "ofxCvMin.h"

#include "ofxCvGui/Panels/Image.h"
#include "ofxCvGui/Widgets/Button.h"

#include "ofxNonLinearFit.h"

using namespace ofxRulr::Nodes;
using namespace ofxCvGui;
using namespace ofxCv;

namespace ofxRulr {
	namespace Procedure {
		namespace Calibrate {
			//----------
			HomographyFromGraycode::HomographyFromGraycode() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void HomographyFromGraycode::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->grid = ofMesh::plane(1.0f, 1.0f, 11.0f, 11.0f);
				for (auto & vertex : grid.getVertices()) {
					vertex += ofVec3f(0.5f, 0.5f, 0.0f);
				}
				
				this->addInput(MAKE(Graph::Pin<Scan::Graycode>));
				this->addInput(MAKE(Graph::Pin<Item::Camera>));

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

				this->undistortFirst.set("Undistort first", false);
				this->doubleExportSize.set("Double size of exported images", false);
			}

			//----------
			string HomographyFromGraycode::getTypeName() const {
				return "Procedure::Calibrate::HomographyFromGraycode";
			}

			//----------
			ofxCvGui::PanelPtr HomographyFromGraycode::getView() {
				return this->view;
			}

			//----------
			void HomographyFromGraycode::update() {
				auto graycodeNode = this->getInput<Scan::Graycode>();
				if (graycodeNode) {
					this->view->setImage(graycodeNode->getDecoder().getProjectorInCamera());
				}
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

				try {
					throwIfMissingAnyConnection();
					auto graycodeNode = this->getInput<Scan::Graycode>();
					auto & dataSet = graycodeNode->getDecoder().getDataSet();
					if (!dataSet.getHasData()) {
						throw(new Exception("No [ofxGraycode::DataSet] loaded"));
					}
					auto normalisedToCamera = ofMatrix4x4::newTranslationMatrix(1.0f, -1.0f, 1.0f) * 
						ofMatrix4x4::newScaleMatrix(0.5f, -0.5f, 1.0f) *
						ofMatrix4x4::newScaleMatrix(dataSet.getWidth(), dataSet.getHeight(), 1.0f);
					auto normaliseToProjector = ofMatrix4x4::newTranslationMatrix(1.0f, -1.0f, 1.0f) *
						ofMatrix4x4::newScaleMatrix(0.5f, -0.5f, 1.0f) *
						ofMatrix4x4::newScaleMatrix(dataSet.getPayloadWidth(), dataSet.getPayloadHeight(), 1.0f);
						;

					auto cameraNormalisedToProjectorNormalised = normalisedToCamera * this->cameraToProjector * normaliseToProjector.getInverse();
					auto projectorNormalisedToCameraNormalised = cameraNormalisedToProjectorNormalised.getInverse();

					auto & jsonCameraToProjectorNormalised = json["cameraNormalisedToProjectorNormalised"];
					auto & jsonProjectorToCameraNormalised = json["projectorNormalisedToCameraNormalised"];
					for (int j = 0; j < 4; j++) {
						auto & jsonCameraToProjectorNormalisedRow = jsonCameraToProjectorNormalised[j];
						auto & jsonProjectorToCameraNormalisedRow = jsonProjectorToCameraNormalised[j];
						for (int i = 0; i < 4; i++) {
							jsonCameraToProjectorNormalisedRow[i] = cameraNormalisedToProjectorNormalised(i, j);
							jsonProjectorToCameraNormalisedRow[i] = projectorNormalisedToCameraNormalised(i, j);
						}
					}
				}
				catch (...)
				{

				}

				Utils::Serializable::serialize(this->undistortFirst, json);
				Utils::Serializable::serialize(this->doubleExportSize, json);
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

				Utils::Serializable::deserialize(this->undistortFirst, json);
				Utils::Serializable::deserialize(this->doubleExportSize, json);
			}

			//----------
			void HomographyFromGraycode::findHomography() {
				this->throwIfMissingAnyConnection();

				auto graycodeNode = this->getInput<Scan::Graycode>();
				auto & dataSet = graycodeNode->getDataSet();
				if (!dataSet.getHasData()) {
					throw(ofxRulr::Exception("No data loaded for [ofxGraycode::DataSet]"));
				}

				vector<ofVec2f> camera;
				vector<ofVec2f> projector;

				for (const auto & pixel : dataSet) {
					if (pixel.active) {
						camera.push_back(pixel.getCameraXY());
						projector.push_back(pixel.getProjectorXY());
					}
				}

				if (this->undistortFirst) {
					this->throwIfMissingAConnection<Item::Camera>();
					auto cameraNode = this->getInput<Item::Camera>();
					camera = toOf(ofxCv::undistortPixelCoordinates(toCv(camera), cameraNode->getCameraMatrix(), cameraNode->getDistortionCoefficients()));
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
						return RULR_CAMERA_DISTORTION_COEFFICIENT_COUNT;
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
			void HomographyFromGraycode::exportMappingImage(string filename) const {
				this->throwIfMissingAnyConnection();

				auto graycodeNode = this->getInput<Scan::Graycode>();
				auto & dataSet = graycodeNode->getDataSet();
				if (!dataSet.getHasData()) {
					throw(ofxRulr::Exception("No data loaded for [ofxGraycode::DataSet]"));
				}

				if (this->cameraToProjector.isIdentity()) {
					throw(ofxRulr::Exception("No mapping has been found yet, so can't save"));
				}

				string filePath;
				if (filename == "") {
					auto filenameBase = ofFilePath::removeExt(dataSet.getFilename());
					auto result = ofSystemSaveDialog(filenameBase + "-cameraToProjector.exr", "Save mapping image");
					if (!result.bSuccess) {
						return;
					}
					filePath = result.filePath;
				}
				else {
					filePath = filename;
				}

				auto mappingGrid = this->grid;
				mappingGrid.clearColors();
				for (auto vertex : mappingGrid.getVertices()) {
					ofFloatColor color;
					(ofVec3f&)color = vertex;
					color.a = 1.0f;
					mappingGrid.addColor(color);
				}

				auto factor = this->doubleExportSize ? 2.0f : 1.0f;
				
				//note that these settings can fail with older versions of openFrameworks,
				// where ofGLUtils.cpp lacks GL_RGBA32F from the ofGetImageTypeFromGLType function
				ofFbo mappingImage;
				ofFbo::Settings settings;
				settings.width = dataSet.getPayloadWidth() * factor;
				settings.height = dataSet.getPayloadHeight() * factor;
				settings.internalformat = GL_RGBA32F;
				settings.numColorbuffers = 1;
				mappingImage.allocate(settings);

				mappingImage.begin();
				ofClear(0, 0);
				
				ofScale(factor, factor, 1.0f);

				ofMultMatrix(this->cameraToProjector);
				ofScale(dataSet.getWidth(), dataSet.getHeight());
				ofPushStyle();
				mappingGrid.drawFaces();
				ofPopStyle();
				ofPopMatrix();

				mappingImage.end();

				ofFloatImage saveImage;
				mappingImage.readToPixels(saveImage.getPixelsRef());
				saveImage.saveImage(filePath + ".exr");


				//save matrix
				ofstream fileOut;
				fileOut.open(ofToDataPath(filePath + ".matrix").c_str(), ofstream::out | ofstream::binary);
				fileOut.write((char*)&this->cameraToProjector, sizeof(this->cameraToProjector));
				fileOut.close();

				//save undistort
				if (this->undistortFirst) {
					this->throwIfMissingAConnection<Item::Camera>();
					auto cameraNode = this->getInput<Item::Camera>();

					auto cameraMatrix = cameraNode->getCameraMatrix();
					auto distortionCoefficients = cameraNode->getDistortionCoefficients();
					auto fx = cameraMatrix.at<double>(0, 0);
					auto fy = cameraMatrix.at<double>(1, 1);
					auto cx = cameraMatrix.at<double>(0, 2);
					auto cy = cameraMatrix.at<double>(1, 2);
					auto distortionVector = vector<double>(RULR_VIEW_DISTORTION_COEFFICIENT_COUNT);
					for (int i = 0; i < distortionVector.size(); i++) {
						distortionVector[i] = distortionCoefficients.at<double>(i);
					}

					ofstream fileOut;
					fileOut.open(ofToDataPath(filePath + ".undistort").c_str(), ofstream::out | ofstream::binary);
					fileOut.write((char*)&fx, sizeof(double));
					fileOut.write((char*)&fy, sizeof(double));
					fileOut.write((char*)&cx, sizeof(double));
					fileOut.write((char*)&cy, sizeof(double));
					for (int i = 0; i < distortionVector.size(); i++) {
						fileOut.write((char*)&distortionVector[i], sizeof(double));
					}
					fileOut.close();
				}
			}

			//----------
			void HomographyFromGraycode::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
				auto findHomographyButton = MAKE(ofxCvGui::Widgets::Button, "Find Homography", [this]() {
					try {
						this->findHomography();
					}
					RULR_CATCH_ALL_TO_ALERT
				}, OF_KEY_RETURN);
				findHomographyButton->setHeight(100.0f);
				inspector->add(findHomographyButton);

				inspector->add(MAKE(ofxCvGui::Widgets::Button, "Export mapping image and matrix...", [this]() {
					try {
						this->exportMappingImage();
					}
					RULR_CATCH_ALL_TO_ALERT
				}));

				inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->undistortFirst));
				inspector->add(MAKE(ofxCvGui::Widgets::Toggle, this->doubleExportSize));
			}
		}
	}
}

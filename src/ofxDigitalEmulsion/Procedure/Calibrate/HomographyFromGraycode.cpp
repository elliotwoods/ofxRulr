#include "HomographyFromGraycode.h"

#include "../../Utils/Exception.h"

#include "../Scan/Graycode.h"
#include "ofxCvMin.h"

#include "ofxCvGui/Panels/Image.h"
#include "ofxCvGui/Widgets/Button.h"

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

			}
		}
	}
}

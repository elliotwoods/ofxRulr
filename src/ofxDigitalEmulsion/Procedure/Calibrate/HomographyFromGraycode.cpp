#include "HomographyFromGraycode.h"

#include "../../Utils/Exception.h"

#include "../Scan/Graycode.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//----------
			HomographyFromGraycode::HomographyFromGraycode() {
				this->inputPins.push_back(MAKE(Graph::Pin<Scan::Graycode>));
				this->view = MAKE(ofxCvGui::Panels::Base);
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

			}

			//----------
			void HomographyFromGraycode::deserialize(const Json::Value & json) {

			}

			//----------
			void HomographyFromGraycode::populateInspector2(ofxCvGui::ElementGroupPtr inspector) {

			}

			//----------
			void HomographyFromGraycode::fit() {
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

				cv::findHomography(ofxCv::toCv(camera), ofxCv::toCv(projector), cv::Mat(), CV_RANSAC, 5.0);
			}
		}
	}
}

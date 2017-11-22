#include "pch_Plugin_ArUco.h"

#include "Detector.h"
#include "ofxRulr/Nodes/GraphicsManager.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			Detector::Detector() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			std::string Detector::getTypeName() const {
				return "ArUco::Detector";
			}

			//----------
			void Detector::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->rebuildDetector();
				aruco::Marker;
				//set the default
				this->parameters.detectorType = DetectorType::MIP_3612h;

				{
					auto panel = ofxCvGui::Panels::makeImage(this->preview);
					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						ofPushStyle();
						{
							ofNoFill();
							ofSetColor(255, 100, 100);
							ofSetLineWidth(2.0f);

							for (const auto & marker : this->foundMarkers) {
								ofPolyline line;
								for (const cv::Point2f & point : marker) {
									line.addVertex(ofxCv::toOf(point));
								}
								line.close();
								line.draw();

								ofDrawBitmapString(ofToString(marker.id), ofxCv::toOf(marker.getCenter()));
							}
						}
						ofPopStyle();
					};
					this->panel = panel;
				}

				//add listeners
				this->parameters.detectorType.addListener(this, &Detector::changeDetectorCallback);
				this->parameters.markerLength.addListener(this, &Detector::changeFloatCallback);
				this->parameters.threshold.areaSize.addListener(this, &Detector::changeIntCallback);
				this->parameters.threshold.subtract.addListener(this, &Detector::changeIntCallback);
				this->parameters.threshold.parameterRange.addListener(this, &Detector::changeIntCallback);
				this->parameters.refinement.refinementType.addListener(this, &Detector::changeRefinementTypeCallback);
				this->parameters.refinement.windowSize.addListener(this, &Detector::changeIntCallback);
			}

			//----------
			void Detector::update() {
				if(this->detectorDirty) {
					this->rebuildDetector();
				}
			}

			//----------
			void Detector::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
				inspector->addLiveValue<string>("Dictionary type", [this]() {
					return aruco::Dictionary::getTypeString(this->dictionaryType);
				});
				inspector->addButton("Retry detect", [this]() {
					this->findMarkers(this->lastDetection.image
						, this->lastDetection.cameraMatrix
						, this->lastDetection.distortionCoefficients);
				});
			}

			//----------
			void Detector::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Detector::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
			}

			//----------
			aruco::MarkerDetector & Detector::getMarkerDetector() {
				return this->markerDetector;
			}

			//----------
			aruco::Dictionary & Detector::getDictionary() {
				return this->dictionary;
			}

			//----------
			const ofImage & Detector::getMarkerImage(int markerIndex) {
				auto findImage = this->cachedMarkerImages.find(markerIndex);
				if (findImage == this->cachedMarkerImages.end()) {
					auto image = make_unique<ofImage>();
					auto mat = this->dictionary.getMarkerImage_id(markerIndex, 8, false);
					ofxCv::copy(mat, image->getPixels());
					image->getPixels().resize(ARUCO_PREVIEW_RESOLUTION, ARUCO_PREVIEW_RESOLUTION, OF_INTERPOLATE_NEAREST_NEIGHBOR);
					image->update();
					image->getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);

					findImage = this->cachedMarkerImages.emplace(markerIndex, move(image)).first;
				}
				return *findImage->second;
			}

			//----------
			float Detector::getMarkerLength() const {
				return this->parameters.markerLength;
			}

			//----------
			ofxCvGui::PanelPtr Detector::getPanel() {
				return this->panel;
			}

			//----------
			const std::vector<aruco::Marker> & Detector::findMarkers(const cv::Mat & image
				, const cv::Mat & cameraMatrix
				, const cv::Mat & distortionCoefficients) {
				this->lastDetection = { image
					, cameraMatrix
					, distortionCoefficients
				};

				if (cameraMatrix.empty()) {
					this->foundMarkers = this->markerDetector.detect(image);
				}
				else {
					aruco::CameraParameters cameraParameters(cameraMatrix
						, distortionCoefficients
						, cv::Size(image.cols, image.rows));
					this->foundMarkers = this->markerDetector.detect(image, cameraParameters, this->parameters.markerLength);
				}
				auto thresholdedImage = this->markerDetector.getThresholdedImage();
				if (!thresholdedImage.empty()) {
					ofxCv::copy(thresholdedImage, this->preview);
					this->preview.update();
				}
				return this->foundMarkers;
			}

			//----------
			void Detector::rebuildDetector() {
				this->markerDetector.setThresholdParams(this->parameters.threshold.areaSize, this->parameters.threshold.subtract);
				this->markerDetector.setThresholdParamRange(this->parameters.threshold.parameterRange, 0);

				switch (this->parameters.detectorType.get())
				{
				case DetectorType::Original:
					this->dictionaryType = aruco::Dictionary::ARUCO;
					break;
				case DetectorType::MIP_3612h:
					this->dictionaryType = aruco::Dictionary::ARUCO_MIP_36h12;
					break;
				case DetectorType::ARTKP:
					this->dictionaryType = aruco::Dictionary::ARTOOLKITPLUS;
					break;
				case DetectorType::ARTAG:
					this->dictionaryType = aruco::Dictionary::ARTAG;
					break;
				default:
					throw(ofxRulr::Exception("Detector type not supported"));
					break;
				}

				this->dictionary = aruco::Dictionary::loadPredefined(this->dictionaryType);
				this->markerDetector.setDictionary(this->dictionaryType);

				//setup refinement method
				{
					aruco::MarkerDetector::CornerRefinementMethod cornerRefinementMethod;
					switch (this->parameters.refinement.refinementType.get()) {

					case RefinementType::SubPix:
						cornerRefinementMethod = aruco::MarkerDetector::CornerRefinementMethod::SUBPIX;
						break;
					case RefinementType::Lines:
						cornerRefinementMethod = aruco::MarkerDetector::CornerRefinementMethod::LINES;
						break;
					case RefinementType::None:
					default:
						cornerRefinementMethod = aruco::MarkerDetector::CornerRefinementMethod::NONE;
						break;
					}
					this->markerDetector.setCornerRefinementMethod(cornerRefinementMethod, this->parameters.refinement.windowSize);
				}

				this->cachedMarkerImages.clear();
				this->detectorDirty = false;
			}

			//----------
			void Detector::changeDetectorCallback(DetectorType &) {
				this->detectorDirty = true;
			}

			//----------
			void Detector::changeFloatCallback(float &) {
				this->detectorDirty = true;
			}

			//----------
			void Detector::changeIntCallback(int &) {
				this->detectorDirty = true;
			}

			//----------
			void Detector::changeRefinementTypeCallback(RefinementType &){
				this->detectorDirty = true;
			}
		}
	}
}
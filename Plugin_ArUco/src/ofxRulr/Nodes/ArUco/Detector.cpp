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
				this->parameters.dictionary = DetectorType::MIP_3612h;

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
									line.addVertex({
										point.x
										, point.y
										, 0.0f
										});
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
				this->parameters.dictionary.addListener(this, &Detector::changeDetectorCallback);
				this->parameters.markerLength.addListener(this, &Detector::changeFloatCallback);
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
			void Detector::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
			}

			//----------
			void Detector::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->parameters);
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
					auto image = make_shared<ofImage>();
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

				this->lastDetection = {
					image.clone()
					, cameraMatrix
					, distortionCoefficients
				};

				if (this->parameters.normalizeImage) {
					cv::normalize(this->lastDetection.image
						, this->lastDetection.image
						, 255
						, 0
						, cv::NormTypes::NORM_MINMAX);
				}

				if (cameraMatrix.empty()) {
					this->foundMarkers = this->markerDetector.detect(this->lastDetection.image);
				}
				else {
					aruco::CameraParameters cameraParameters(cameraMatrix
						, distortionCoefficients
						, cv::Size(image.cols, image.rows));
					this->foundMarkers = this->markerDetector.detect(this->lastDetection.image, cameraParameters, this->parameters.markerLength);
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
				switch (this->parameters.dictionary.get())
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

				auto & parameters = this->markerDetector.getParameters();
				parameters.enclosedMarker = this->parameters.enclosedMarkers.get();
				parameters.NAttemptsAutoThresFix = this->parameters.thresholdAttempts.get();

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
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

				//set the default
				this->parameters.detectorType = DetectorType::MIP_3612h;

				this->panel = ofxCvGui::Panels::makeImage(this->preview);
			}

			//----------
			void Detector::update() {
				if (this->cachedDetector != this->parameters.detectorType.get()) {
					this->rebuildDetector();
				}

				//update preview
				{
					auto thresholdedImage = this->markerDetector.getThresholdedImage();
					if (!thresholdedImage.empty()) {
						ofxCv::copy(thresholdedImage, this->preview);
						this->preview.update();
					}
				}
			}

			//----------
			void Detector::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
				inspector->addLiveValue<string>("Dictionary type", [this]() {
					return aruco::Dictionary::getTypeString(this->dictionaryType);
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
				this->cachedDetector = this->parameters.detectorType;

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
			}
		}
	}
}
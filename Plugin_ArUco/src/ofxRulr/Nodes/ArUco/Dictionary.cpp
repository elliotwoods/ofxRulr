#include "pch_Plugin_ArUco.h"

#include "Dictionary.h"
#include "ofxRulr/Nodes/GraphicsManager.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//---------
			Dictionary::Dictionary() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			string Dictionary::getTypeName() const {
				return "ArUco::Dictionary";
			}

			//----------
			void Dictionary::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_UPDATE_LISTENER;
			}

			//----------
			void Dictionary::update() {
				if (this->getDictionaryType() != this->cachedDictionary) {
					this->rebuildDictionary();
				}
			}

			//----------
			void Dictionary::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
				inspector->addLiveValue<string>("Dictionary name", [this]() {
					return this->getDictionaryName();
				});
				/*
				TODO -
				auto typeWidget = ofxCvGui::Panels::WidgetsBuilder::X().tryBuild(this->parameters.dictionaryType.newReference());
				inspector->add(typeWidget);

				auto resolutionWidget = ofxCvGui::Panels::WidgetsBuilder::X().tryBuild(this->parameters.markerResolution.newReference());
				inspector->add(resolutionWidget);

				auto dictionarySizeWidget = ofxCvGui::Panels::WidgetsBuilder::X().tryBuild(this->parameters.dictionarySize.newReference());
				inspector->add(dictionarySizeWidget);

				typeWidget->onUpdate += [this, resolutionWidget, dictionarySizeWidget](ofxCvGui::UpdateArguments &) {
					bool isPredefined = this->parameters.dictionaryType.get() == DictionaryType::Predefined;
					resolutionWidget->setEnabled(isPredefined);
					dictionarySizeWidget->setEnabled(isPredefined);
				};
				*/
			}

			//----------
			void Dictionary::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
			}

			//----------
			void Dictionary::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->parameters);
			}

			//----------
			cv::aruco::PREDEFINED_DICTIONARY_NAME Dictionary::getDictionaryType() const {
				if (this->parameters.dictionaryType.get() == DictionaryType::Original)
				{
					return cv::aruco::DICT_ARUCO_ORIGINAL;
				}
				else {
					switch (this->parameters.markerResolution.get().get()) {
					case MarkerResolution::_4x4:
						switch (this->parameters.dictionarySize.get())
						{
						case DictionarySize::_50:
							return cv::aruco::DICT_4X4_50;
						case DictionarySize::_100:
							return cv::aruco::DICT_4X4_100;
						case DictionarySize::_250:
							return cv::aruco::DICT_4X4_250;
						case DictionarySize::_1000:
							return cv::aruco::DICT_4X4_1000;
						default:
							break;
						}
						break;
					case MarkerResolution::_5x5:
						switch (this->parameters.dictionarySize.get())
						{
						case DictionarySize::_50:
							return cv::aruco::DICT_5X5_50;
						case DictionarySize::_100:
							return cv::aruco::DICT_5X5_100;
						case DictionarySize::_250:
							return cv::aruco::DICT_5X5_250;
						case DictionarySize::_1000:
							return cv::aruco::DICT_5X5_1000;
						default:
							break;
						}
						break;
					case MarkerResolution::_6x6:
						switch (this->parameters.dictionarySize.get())
						{
						case DictionarySize::_50:
							return cv::aruco::DICT_6X6_50;
						case DictionarySize::_100:
							return cv::aruco::DICT_6X6_100;
						case DictionarySize::_250:
							return cv::aruco::DICT_6X6_250;
						case DictionarySize::_1000:
							return cv::aruco::DICT_6X6_1000;
						default:
							break;
						}
						break;
					case MarkerResolution::_7x7:
						switch (this->parameters.dictionarySize.get())
						{
						case DictionarySize::_50:
							return cv::aruco::DICT_7X7_50;
						case DictionarySize::_100:
							return cv::aruco::DICT_7X7_100;
						case DictionarySize::_250:
							return cv::aruco::DICT_7X7_250;
						case DictionarySize::_1000:
							return cv::aruco::DICT_7X7_1000;
						default:
							break;
						}
						break;
					default:
						break;
					}
				}
				throw(ofxRulr::Exception("ArUco board type not supported"));
			}

			//----------
			std::string Dictionary::getDictionaryName() const {
				switch (this->getDictionaryType()) {
				case cv::aruco::DICT_4X4_50:
					return "DICT_4X4_50";
				case cv::aruco::DICT_4X4_100:
					return "DICT_4X4_50";
				case cv::aruco::DICT_4X4_250:
					return "DICT_4X4_250";
				case cv::aruco::DICT_4X4_1000:
					return "DICT_4X4_1000";
				case cv::aruco::DICT_5X5_50:
					return "DICT_5X5_50";
				case cv::aruco::DICT_5X5_100:
					return "DICT_5X5_100";
				case cv::aruco::DICT_5X5_250:
					return "DICT_5X5_250";
				case cv::aruco::DICT_5X5_1000:
					return "DICT_5X5_1000";
				case cv::aruco::DICT_6X6_50:
					return "DICT_6X6_50";
				case cv::aruco::DICT_6X6_100:
					return "DICT_6X6_100";
				case cv::aruco::DICT_6X6_250:
					return "DICT_6X6_250";
				case cv::aruco::DICT_6X6_1000:
					return "DICT_6X6_1000";
				case cv::aruco::DICT_7X7_50:
					return "DICT_7X7_50";
				case cv::aruco::DICT_7X7_100:
					return "DICT_7X7_100";
				case cv::aruco::DICT_7X7_250:
					return "DICT_7X7_250";
				case cv::aruco::DICT_7X7_1000:
					return "DICT_7X7_1000";
				case cv::aruco::DICT_ARUCO_ORIGINAL:
					return "DICT_ARUCO_ORIGINAL";
				default:
					return "UNKNOWN";
					break;
				}
			}

			//----------
			cv::Ptr<cv::aruco::Dictionary> Dictionary::getDictionary() const {
				return this->dictionary;
			}

			//----------
			void Dictionary::rebuildDictionary() {
				auto dictionaryType = this->getDictionaryType();
				this->dictionary = cv::aruco::getPredefinedDictionary(dictionaryType);
				this->cachedDictionary = dictionaryType;
			}

			//----------
			float Dictionary::getMarkerSize() const {
				return this->parameters.markerSize;
			}
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"
#include <opencv2/aruco.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class Dictionary : public Nodes::Base {
			public:
				Dictionary();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				cv::aruco::PREDEFINED_DICTIONARY_NAME getDictionaryType() const;
				string getDictionaryName() const;
				cv::Ptr<cv::aruco::Dictionary> getDictionary() const;

				float getMarkerSize() const;
			protected:
				MAKE_ENUM(DictionaryType
					, (Original, Predefined)
					, ("Original", "Predefined"));
				MAKE_ENUM(MarkerResolution
					, (_4x4, _5x5, _6x6, _7x7)
					, ("4x4", "5x5", "6x6", "7x7"));
				MAKE_ENUM(DictionarySize
					, (_50, _100, _250, _1000)
					, ("50", "100", "250", "1000"));

				void rebuildDictionary();

				struct : ofParameterGroup {
					ofParameter<DictionaryType> dictionaryType{ "Dictionary type", DictionaryType::Original };
					ofParameter<MarkerResolution> markerResolution{ "Marker resolution", MarkerResolution::_6x6 };
					ofParameter<DictionarySize> dictionarySize{ "Dictionary size", DictionarySize::_250 };
					ofParameter<float> markerSize{ "Marker size [m]", 0.05, 0.001, 10 };
					PARAM_DECLARE("Dictionary", dictionaryType, markerResolution, dictionarySize, markerSize);
				} parameters;

				cv::Ptr<cv::aruco::Dictionary> dictionary;
				cv::aruco::PREDEFINED_DICTIONARY_NAME cachedDictionary = (cv::aruco::PREDEFINED_DICTIONARY_NAME) -1;
			};
		}
	}
}
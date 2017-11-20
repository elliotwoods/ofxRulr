#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include <aruco.h>

#define ARUCO_PREVIEW_RESOLUTION 64

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class Detector : public Nodes::Base {
			public:
				Detector();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				aruco::MarkerDetector & getMarkerDetector();
				aruco::Dictionary & getDictionary();
				const ofImage & getMarkerImage(int markerIndex);

				float getMarkerLength() const;

				ofxCvGui::PanelPtr getPanel() override;
				
				const vector<aruco::Marker> & findMarkers(const cv::Mat &, shared_ptr<Item::View> camera = nullptr);
			protected:
				MAKE_ENUM(DetectorType
					, (Original, MIP_3612h, ARTKP, ARTAG)
					, ("Original", "MIP_3612h", "ARTK+", "ARTAG"));

				MAKE_ENUM(RefinementType
					, (None, SubPix, Lines)
					, ("None", "SubPix", "Lines"));

				void rebuildDetector();

				struct : ofParameterGroup {
					ofParameter<DetectorType> detectorType{ "Detector type", DetectorType::Original };
					ofParameter<float> markerLength{ "Marker length [m]", 0.05, 0.001, 10 };
					
					struct : ofParameterGroup {
						ofParameter<int> areaSize{ "Area size", 7 };
						ofParameter<int> subtract{ "Subtract", 7 };
						ofParameter<int> parameterRange{ "Parameter range", 2 };
						PARAM_DECLARE("Threshold", areaSize, subtract, parameterRange);
					} threshold;

					struct : ofParameterGroup {
						ofParameter<RefinementType> refinementType{ "Refinement type", RefinementType::SubPix };
						ofParameter<int> windowSize{ "Window size", 3 };
						PARAM_DECLARE("Refinement", refinementType, windowSize);
					} refinement;

					PARAM_DECLARE("Dictionary", detectorType, markerLength, threshold, refinement);
				} parameters;

				aruco::Dictionary dictionary;
				aruco::Dictionary::DICT_TYPES dictionaryType;
				aruco::MarkerDetector markerDetector;
				DetectorType cachedDetector;
				RefinementType cachedRefinement;

				map<int, unique_ptr<ofImage>> cachedMarkerImages;

				ofImage preview;
				vector<aruco::Marker> foundMarkers;
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
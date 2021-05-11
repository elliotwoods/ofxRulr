#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include <aruco/aruco.h>

#define ARUCO_PREVIEW_RESOLUTION 64

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class PLUGIN_ARUCO_EXPORTS Detector : public Nodes::Base {
			public:
				Detector();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				aruco::MarkerDetector & getMarkerDetector();
				aruco::Dictionary & getDictionary();
				const ofImage & getMarkerImage(int markerIndex);

				float getMarkerLength() const;

				ofxCvGui::PanelPtr getPanel() override;
				
				const vector<aruco::Marker> & findMarkers(const cv::Mat & image);
			protected:
				MAKE_ENUM(DetectorType
					, (Original, MIP_3612h, ARTKP, ARTAG)
					, ("Original", "MIP_3612h", "ARTK+", "ARTAG"));

				MAKE_ENUM(RefinementType
					, (None, SubPix, Lines)
					, ("None", "SubPix", "Lines"));

				void rebuildDetector();

				void changeDetectorCallback(DetectorType &);
				void changeFloatCallback(float &);
				void changeIntCallback(int &);
				void changeRefinementTypeCallback(RefinementType &);

				struct : ofParameterGroup {
					ofParameter<DetectorType> dictionary{ "Detector type", DetectorType::Original };
					ofParameter<float> markerLength{ "Marker length [m]", 0.05, 0.001, 10 };
					ofParameter<bool> normalizeImage{ "Normalize image", false };
					ofParameter<bool> enclosedMarkers{ "Enclosed markers", false };
					ofParameter<int> thresholdAttempts{ "Threshold attempts", 3, 1, 10 };
					ofParameter<float> cornerRefineZone{ "Corner refine zone %", 0.05 ,0, 1 }; // This value works well with our halo markers
					PARAM_DECLARE("Detector", dictionary, markerLength, normalizeImage, enclosedMarkers, thresholdAttempts, cornerRefineZone);
				} parameters;

				aruco::Dictionary dictionary;
				aruco::Dictionary::DICT_TYPES dictionaryType;
				aruco::MarkerDetector markerDetector;

				map<int, shared_ptr<ofImage>> cachedMarkerImages;

				//useful when debugging to research quickly
				struct {
					cv::Mat image;
				} lastDetection;

				ofImage preview;
				vector<aruco::Marker> foundMarkers;
				ofxCvGui::PanelPtr panel;

				bool detectorDirty = true;
			};
		}
	}
}
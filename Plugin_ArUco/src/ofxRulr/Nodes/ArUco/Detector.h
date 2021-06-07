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

				MAKE_ENUM(Preview
					, (Raw, Normalised, Thresholded, None)
					, ("Raw", "Normalised", "Thresholded", "None"));

				void rebuildDetector();
				void buildDetector(aruco::MarkerDetector&, aruco::Dictionary&, aruco::Dictionary::DICT_TYPES&) const;

				void changeDetectorCallback(DetectorType &);
				void changeFloatCallback(float &);
				void changeIntCallback(int&);
				void changeBoolCallback(bool &);

				struct : ofParameterGroup {
					ofParameter<DetectorType> dictionary{ "Detector type", DetectorType::Original };
					ofParameter<float> markerLength{ "Marker length [m]", 0.05, 0.001, 10 };

					struct : ofParameterGroup {
						ofParameter<int> threads{ "Threads", 8};
						ofParameter<bool> enclosedMarkers{ "Enclosed markers", false };
						ofParameter<int> thresholdAttempts{ "Threshold attempts", 3, 1, 10 };
						struct : ofParameterGroup {
							ofParameter<int> windowSize{ "Window Size", -1 };
							ofParameter<int> windowSizeRange{ "Window Size Range", 0 };
							PARAM_DECLARE("Adaptive threshold", windowSize, windowSizeRange);
						} adaptiveThreshold;
						ofParameter<int> threshold{ "Threshold", 7 };
						PARAM_DECLARE("ArUco Detector"
							, threads
							, enclosedMarkers
							, thresholdAttempts
							, adaptiveThreshold
							, threshold
						);
					} arucoDetector;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							PARAM_DECLARE("Normalize", enabled);
						} normalize;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> iterations{ "Iterations", 4, 1, 10 };
							ofParameter<float> overlap{ "Overlap", 0.5, 0.1, 1.0 };
							PARAM_DECLARE("Multi crop", enabled, iterations, overlap);
						} multiCrop;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> maxBrightess{ "Max brightness [x]", 4};
							PARAM_DECLARE("Multi brightness", enabled, maxBrightess);
						} multiBrightness;

						PARAM_DECLARE("Strategies", normalize, multiCrop, multiBrightness)
					} strategies;
					

					struct : ofParameterGroup {
						ofParameter<float> zone1{ "Zone 1 %", 0.075, 0, 1 }; // This value works well with our halo markers
						ofParameter<float> zone2{ "Zone 2 %", 0.02, 0, 1 }; // This value works well with our halo markers
						PARAM_DECLARE("Corner refinement", zone1, zone2);
					} cornerRefinement;
					
					struct : ofParameterGroup {
						ofParameter<bool> speakCount{ "Speak count", true };
						ofParameter<Preview> preview{ "Preview", Preview::Raw };
						PARAM_DECLARE("Debug", speakCount)
					} debug;

					PARAM_DECLARE("Detector"
						, dictionary
						, markerLength
						, arucoDetector
						, strategies
						, cornerRefinement
						, debug);
				} parameters;

				aruco::Dictionary dictionary;
				aruco::Dictionary::DICT_TYPES dictionaryType;
				aruco::MarkerDetector markerDetector;

				map<int, shared_ptr<ofImage>> cachedMarkerImages;

				//useful when debugging to research quickly
				struct {
					cv::Mat thresholded;
					cv::Mat normalisedImage;
					cv::Mat rawImage;
				} lastDetection;

				ofImage preview;
				Preview cachedPreviewType = Preview::Raw;

				vector<aruco::Marker> foundMarkers;
				ofxCvGui::PanelPtr panel;

				bool detectorDirty = true;
			};
		}
	}
}
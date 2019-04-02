#pragma once

#include "pch_Plugin_LSS.h"

// Note : this is for the monocular method (for stereo see FitLines)

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class FitPointsToLines : public Nodes::Base {
			public:
				struct ProjectorPixel {
					ofVec2f projector;

					vector<ofVec2f> cameraFinds;
					ofVec2f camera;
					ofVec2f cameraUndistorted;

					ofVec3f world;
					float intersectionLength;

					bool unavailable = false;
				};

				struct Line {
					vector<shared_ptr<ProjectorPixel>> projectorPixels;
					ofVec3f startWorld;
					ofVec3f endWorld;

					ofVec2f startProjector;
					ofVec2f endProjector;
				};

				FitPointsToLines();
				string getTypeName() const override;
				void init();
				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments &);

				void gatherProjectorPixels();
				void triangulate();
				void fit();
			protected:
				ofxRay::Ray fitRayToProjectorPixels(vector<shared_ptr<ProjectorPixel>> projectorPixels);

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> distanceThreshold{ "Distance threshold", 10, 0, 255 };
						PARAM_DECLARE("Gather projector pixels", distanceThreshold);
					} gatherProjectorPixels;

					struct : ofParameterGroup {
						ofParameter<float> maxCameraDeviation{ "Max camera deviation [px]", 3, 0, 100 };
						ofParameter<float> maxIntersectionLength{ "Max intersection length [m]", 0.03, 0, 1 };
						PARAM_DECLARE("Triangulate", maxCameraDeviation, maxIntersectionLength);
					} triangulate;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> sizeM{ "Size [m]", 0.2, 0, 1.0 };
							ofParameter<float> sizePx{ "Size [px]", 50, 0, 100 };
							ofParameter<float> minimumCount{ "Minimum count [px]", 20, 1, 1000 };
							ofParameter<float> deviationThreshold{ "Deviation threshold [m]", 0.025, 0, 1.0 };
							PARAM_DECLARE("Head", sizeM, sizePx, minimumCount, deviationThreshold);
						} head;

						struct : ofParameterGroup
						{
							ofParameter<float> distance{ "Distance", 0.02 };
							ofParameter<float> sizeM{ "Size [m]", 0.2, 0, 1.0 };
							ofParameter<float> minimumCount{ "Minimum count [px]", 20, 1, 1000 };
							PARAM_DECLARE("Walk", distance, sizeM, minimumCount);
						} walk;

						struct : ofParameterGroup {
							ofParameter<float> minimumLength{ "Minimum length [m]", 0.1, 0, 10 };
							ofParameter<float> deviationThreshold{ "Deviation threshold [m]", 0.025, 0, 1.0 };
							ofParameter<float> deviantPopulationLimit{ "Deviant population limit [%]", 0.1, 0, 1 };
							PARAM_DECLARE("Test", minimumLength, deviationThreshold, deviantPopulationLimit);
						} test;

						PARAM_DECLARE("Search", head, walk, test);
					} search;

					struct : ofParameterGroup {
						ofParameter<bool> unclassifiedPixels{ "Unclassified pixels", true };
						ofParameter<bool> lines{ "Lines", true };
						ofParameter<bool> debug{ "Debug", true };
						PARAM_DECLARE("Preview", unclassifiedPixels, lines, debug);
					} preview;

					PARAM_DECLARE("FitPointsToLines", gatherProjectorPixels, triangulate, search, preview);
				} parameters;

				map<uint32_t, shared_ptr<ProjectorPixel>> unclassifiedPixels; // indexed by projector pixel
				vector<shared_ptr<Line>> lines;

				ofMesh previewUnclassifiedPixels;
				ofMesh previewLines;

				ofxRay::Ray debugRay;
				vector<ofVec3f> debugPoints;
			};
		}
	}
}
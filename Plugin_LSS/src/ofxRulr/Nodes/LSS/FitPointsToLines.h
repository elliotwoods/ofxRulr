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

					bool triangulationFailed = false;
					bool unavailable = false;
				};

				struct Line {
					Line();

					vector<shared_ptr<ProjectorPixel>> projectorPixels;
					ofVec3f startWorld;
					ofVec3f endWorld;

					ofVec2f startProjector;
					ofVec2f endProjector;

					ofColor color;
				};

				FitPointsToLines();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();
				ofxCvGui::PanelPtr getPanel() override;

				void populateInspector(ofxCvGui::InspectArguments &);

				void calibrate();
				void gatherProjectorPixels();
				void triangulate();
				void fit();
				void projectTo2D();
				void exportData();
			protected:
				ofxRay::Ray fitRayToProjectorPixels(vector<shared_ptr<ProjectorPixel>> projectorPixels);
				vector<shared_ptr<ProjectorPixel>> findNClosestPixels(const vector<shared_ptr<ProjectorPixel>> projectorPixels, const ofVec3f & position, int count);
				ofVec2f findMeanClosestProjected(const vector<shared_ptr<ProjectorPixel>> & projectorPixels, const ofVec3f & worldPosition);
				ofVec3f findMeanClosestWorld(const vector<shared_ptr<ProjectorPixel>> & projectorPixels, const ofVec3f & worldPosition);

				void rebuildPreview();
				void drawOnProjector();

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
							ofParameter<float> sizeM{ "Size [m]", 0.05, 0, 1.0 };
							ofParameter<float> sizePx{ "Size [px]", 50, 0, 100 };
							ofParameter<float> minimumCount{ "Minimum count [px]", 20, 1, 1000 };
							ofParameter<float> deviationThreshold{ "Deviation threshold [m]", 0.025, 0, 1.0 };
							PARAM_DECLARE("Head", sizeM, sizePx, minimumCount, deviationThreshold);
						} head;

						struct : ofParameterGroup
						{
							ofParameter<float> length{ "Length [m]", 0.02, 0, 1.0 };
							ofParameter<float> width{ "Width [m]", 0.01, 0, 1.0 };
							ofParameter<float> minimumCount{ "Minimum count [px]", 1, 1, 1000 };
							PARAM_DECLARE("Walk", length, width, minimumCount);
						} walk;

						struct : ofParameterGroup {
							ofParameter<float> minimumLength{ "Minimum length [m]", 0.1, 0, 10 };
							ofParameter<float> deviationThreshold{ "Deviation threshold [m]", 0.025, 0, 1.0 };
							ofParameter<float> deviantPopulationLimit{ "Deviant population limit [%]", 0.1, 0, 1 };
							ofParameter<int> maxLineCount{ "Max line count", 300 };
							PARAM_DECLARE("Test", minimumLength, deviationThreshold, deviantPopulationLimit, maxLineCount);
						} test;

						PARAM_DECLARE("Search", head, walk, test);
					} search;

					struct : ofParameterGroup {
						ofParameter<float> deviationThreshold{ "Deviation threshold [px]", 2, 0, 50 };
						ofParameter<int> closePixelCount{"Close pixel count", 5, 1, 100 };
						PARAM_DECLARE("Project to 2D", deviationThreshold, closePixelCount);
					} projectTo2D;

					struct : ofParameterGroup {
						ofParameter<bool> unclassifiedPixels{ "Unclassified pixels", true };
						ofParameter<bool> lines{ "Lines", true };
						ofParameter<bool> classifiedPixels{ "Classified pixels", true };
						ofParameter<bool> onProjector{ "On projector", true };
						ofParameter<bool> debug{ "Debug", true };
						PARAM_DECLARE("Preview", unclassifiedPixels, lines, classifiedPixels, debug);
					} preview;

					PARAM_DECLARE("FitPointsToLines", gatherProjectorPixels, triangulate, search, projectTo2D, preview);
				} parameters;

				ofxCvGui::PanelPtr panel;

				map<uint32_t, shared_ptr<ProjectorPixel>> unclassifiedPixels; // indexed by projector pixel
				vector<shared_ptr<Line>> lines;

				bool previewDirty = true;
				ofMesh previewUnclassifiedPixels;
				ofMesh previewClassifiedPixels;
				ofMesh previewLines;
				ofFbo previewInProjector;

				ofxRay::Ray debugRay;
				vector<ofVec3f> debugPoints;
			};
		}
	}
}
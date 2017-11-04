#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			typedef int MarkerID;

			class Body : public Nodes::Item::RigidBody {
			public:
				struct Description {
					struct Markers {
						vector<MarkerID> IDs;
						vector<ofVec3f> positions;
						vector<ofColor> colors;
					} markers;

					size_t markerCount;
					float markerDiameter;

					ofMatrix4x4 modelTransform;
					cv::Mat rotationVector;
					cv::Mat translation;
				};

				Body();
				string getTypeName() const override;

				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
				void drawObject() const;
				void drawWorldStage() const;

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				//thread safe
				shared_ptr<Description> getBodyDescription() const;

				void addMarker(const ofVec3f &);

				float getMarkerDiameter() const;

				void loadCSV(string filename = "");
				void saveCSV(string filename = "");

				bool getTrackingPrediction(cv::Mat & rotationVector, cv::Mat & translation); // returns true if prediction is used
			protected:
				class Marker : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Marker();
					string getDisplayString() const override;
					ofParameter<MarkerID> ID{ "ID", 0 };
					ofParameter<ofVec3f> position{ "Position", ofVec3f() };
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					deque<ofVec3f> worldHistory;
				};

				struct : ofParameterGroup {
					ofParameter<float> markerDiameter{ "Marker diameter [m]", 0.02, 0.001, 1 };
					
					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> processNoise{ "Process noise", 1e-5, 0, 1 };
						ofParameter<float> measurementNoise{ "Measurement noise", 1e-2, 0, 1 };
						ofParameter<float> errorPost{ "Error post", 1, 0, 10 };
						PARAM_DECLARE("Kalman filter", enabled, processNoise, measurementNoise, errorPost);
					} kalmanFilter;

					struct : ofParameterGroup {
						ofParameter<bool> useColors{ "Use colors", false };
						ofParameter<bool> showIDs{ "Show IDs", false };
						ofParameter<float> historyTrailLength{ "History trail length [s]", 2, 0, 10 };
						PARAM_DECLARE("Draw style (world)", useColors, showIDs, historyTrailLength)
					} drawStyleWorld;
					PARAM_DECLARE("Body", markerDiameter, kalmanFilter, drawStyleWorld);
				} parameters;

				void addMarker();
				MarkerID getNextAvailableID() const;

				void invalidateBodyDescription();

				ofxCvGui::PanelPtr panel;
				Utils::CaptureSet<Marker> markers;

				shared_ptr<Description> bodyDescription;
				mutable mutex bodyDescriptionMutex;

				ofThreadChannel<ofMatrix4x4> transformIncoming;

				friend Marker;
			};
		}
	}
}
#pragma once

#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class SolarAlignment : public Nodes::Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						Capture();
						string getDisplayString() const override;
						void drawWorld();

						ofVec2f centroid;
						ofxRay::Ray ray;
						ofVec2f azimuthAltitude;
						ofVec3f solarVectorFromPySolar;

						void serialize(Json::Value &);
						void deserialize(const Json::Value &);

						void draw();

						struct {
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> worldPoints;
							ofParameter<float> reprojectionError{ "Marker map reprojection error", 0.0f };
							ofParameter<ofVec3f> cameraPosition{ "Camera position", ofVec3f() };
						} cameraNavigation;
					};

					SolarAlignment();
					string getTypeName() const override;

					void init();

					void populateInspector(ofxCvGui::InspectArguments &);
					void serialize(Json::Value &);
					void deserialize(const Json::Value &);

					ofxCvGui::PanelPtr getPanel() override;
					void drawWorldStage();

					void addCapture();
					void calibrate();
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<int> minimumMarkers{ "Minimum markers", 3 };
							ofParameter<float> maxMarkerError{ "Max marker error", 0.5f };
							PARAM_DECLARE("Camera navigation", minimumMarkers, maxMarkerError);
						} cameraNavigation;

						struct : ofParameterGroup {
							ofParameter<float> threshold{ "Threshold", 100, 0, 255 };
							PARAM_DECLARE("Solar centroid", threshold);
						} solarCentroid;

						struct : ofParameterGroup {
							ofParameter<string> sunTrackingScript{ "sunPosition.pyv", "C:\\Users\\kimchips\\Dropbox (Kimchi and Chips)\\KC31 - Halo v2.0 Technical\\Sun Tracking\\sunPosition.py" };
							ofParameter<string> python{ "python exe", "C:\\Users\\kimchips\\Anaconda3\\envs\\KC31\\python.exe" };
							PARAM_DECLARE("Solar tracking", sunTrackingScript, python);
						} solarTracking;

						PARAM_DECLARE("SolarAlignment", cameraNavigation, solarCentroid, solarTracking);
					} parameters;

					Utils::CaptureSet<Capture> captures;
					shared_ptr<ofxCvGui::Panels::Base> panel;
					ofImage preview;
				};
			}
		}
	}
}
#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace SolveMirror {
				class SolveMirror : public Base {
				public:
					class Capture : public Utils::AbstractCaptureSet::BaseCapture {
					public:
						void drawWorld();
						string getDisplayString() const override;

						struct {
							vector<cv::Point2f> imagePoints;
							vector<cv::Point3f> worldPoints;
							ofParameter<float> reprojectionError;
						} cameraNavigation;

						struct {
							vector<cv::Point3f> realPoints;
							vector<cv::Point3f> virtualPoints;
							vector<cv::Point3f> pointOnMirror;
							ofParameter<float> reprojectionError;

							vector<ofxRay::Ray> viewRays;
						} reflections;
					};

					struct PlaneDataPoint {
						ofVec3f realPosition;
						ofVec3f virtualPosition;
					};

					class PlaneModel : public ofxNonLinearFit::Models::Base<PlaneDataPoint, PlaneModel> {
					public:
						unsigned int getParameterCount() const override;
						void getResidual(PlaneDataPoint, double & residual, double * gradient /* = 0 */) const override;
						void evaluate(PlaneDataPoint &) const override;
						void cacheModel() override;
						
						ofxRay::Plane plane;
					};

					SolveMirror();
					string getTypeName() const override;
					void init();
					void update();
					void addCapture();
					void calibrate();
					ofxCvGui::PanelPtr getPanel() override;
					void populateInspector(ofxCvGui::InspectArguments &);
					void drawWorldStage();
				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<int> minimumMarkers { "Minimum markers", 3 };
							PARAM_DECLARE("Camera navigation", minimumMarkers);
						} cameraNavigation;

						ofParameter<float> noise{ "Noise [px]", 0, 0, 100.0 };
						PARAM_DECLARE("SolveMirror", cameraNavigation);
					} parameters;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					Utils::CaptureSet<Capture> captures;

					PlaneModel mirrorPlaneModel;
					ofParameter<float> planeFitResidual{ "Plane fit residual [m]", 0.0f };
				};
			}
		}
	}
}
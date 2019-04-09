#pragma once

#include "ofxRulr.h"
#include "ofxNonLinearFit.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace ProCamSolve {
				class SolveProjector : public Base {
				public:
					SolveProjector();
					string getTypeName() const override;
					void init();
					void populateInspector(ofxCvGui::InspectArguments &);

					void calibrate();

					struct DataPoint {
						ofVec2f cameraUndistorted;
						ofVec2f projector;
					};

					class Model : public ofxNonLinearFit::Models::Base<DataPoint, Model> {
					public:
						struct System {
							cv::Mat cameraMatrix;
							cv::Mat translation;
							cv::Mat rotationVector;
						};

						Model();
						unsigned int getParameterCount() const override;
						void getResidual(DataPoint dataPoint, double & residual, double * gradient) const override;
						void evaluate(DataPoint & dataPoint) const override;
						void cacheModel() override;
						void resetParameters() override;

						shared_ptr<Item::Camera> camera;
						shared_ptr<Item::Projector> projector;
						
						System current;
					protected:
						System startingParameters;
						ofxRay::Camera cameraView;
						ofxRay::Projector projectorView;
					};

				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> threshold{ "Threshold", 2, 0, 255 };
							ofParameter<int> decimate{ "Decimate", 100, 1, 1000 };
							PARAM_DECLARE("Data set", threshold, decimate);
						} dataSet;

						struct : ofParameterGroup {
							ofParameter<bool> retainDistance{ "Retain distance", true };
							PARAM_DECLARE("Treatment", retainDistance);
						} treatment;

						PARAM_DECLARE("BundlerCamera", dataSet, treatment);
					} parameters;
				};
			}
		}
	}
}
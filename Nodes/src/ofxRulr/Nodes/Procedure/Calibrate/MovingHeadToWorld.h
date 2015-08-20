#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"
#include "ofxNonLinearFit.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class MovingHeadToWorld : public Procedure::Base {
				public:
					struct DataPoint {
						ofVec3f world;
						ofVec2f panTilt;
						float residual;
						ofVec2f panTiltEvaluated;
					};

					class Model : public ofxNonLinearFit::Models::Base<MovingHeadToWorld::DataPoint, Model> {
					public:
						Model(); //Fit needs to call this constructor at the start to get parameter count
						Model(const ofVec3f & initialPosition, const ofVec3f & initialRotationEuler);

						unsigned int getParameterCount() const;
						void resetParameters() override;
						double getResidual(DataPoint point) const override;
						void evaluate(DataPoint & point) const;
						void cacheModel() override;

						const ofMatrix4x4 & getTransform();
					protected:
						ofVec3f initialPosition;
						ofVec3f initialRotationEuler;
						ofMatrix4x4 transform;
						float tiltOffset;
					};

					MovingHeadToWorld();
					void init();
					string getTypeName() const override;
					void update();

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::ElementGroupPtr);

					void drawWorld() override;
					ofxCvGui::PanelPtr getView() override;
					void addCapture();
					void calibrate();
				protected:
					vector<DataPoint> dataPoints;
					float residual;

					ofxCvGui::PanelPtr view;
					float lastFind;
				};
			}
		}
	}
}
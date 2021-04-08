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
						glm::vec3 world;
						glm::vec2 panTilt;
						float residual;
						glm::vec2 panTiltEvaluated;
					};

					class Model : public ofxNonLinearFit::Models::Base<MovingHeadToWorld::DataPoint, Model> {
					public:
						Model(); //Fit needs to call this constructor at the start to get parameter count
						Model(const glm::vec3 & initialPosition, const glm::vec3 & initialRotationEuler);

						unsigned int getParameterCount() const;
						void resetParameters() override;
						void getResidual(DataPoint, double & residual, double * gradient) const override;
						void evaluate(DataPoint & point) const;
						void cacheModel() override;

						const glm::mat4 & getTransform();
						float getTiltOffset() const;
					protected:
						glm::vec3 initialPosition;
						glm::vec3 initialRotationEuler;
						glm::mat4 transform;
						float tiltOffset;
					};

					MovingHeadToWorld();
					void init();
					string getTypeName() const override;
					void update();

					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					void populateInspector(ofxCvGui::InspectArguments &);

					void drawWorldStage();
					ofxCvGui::PanelPtr getPanel() override;
					void addCapture();
					void deleteLastCapture();
					bool calibrate(int iterations = 3);
					void performAim();
				protected:
					void setPanTiltOrAlert(const glm::vec2 &);
					vector<DataPoint> dataPoints;
					float residual;

					ofxCvGui::PanelPtr view;
					float lastFindTime;
					ofParameter<float> beamBrightness;
					ofParameter<bool> calibrateOnAdd;
					ofParameter<bool> continuouslyTrack;
				};
			}
		}
	}
}
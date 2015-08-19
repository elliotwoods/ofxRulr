#pragma once

#include "ofxRulr/Nodes/Procedure/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				class MovingHeadToWorld : public Procedure::Base {
				public:
					struct DataPoint {
						ofVec3f world;
						ofVec2f panTilt;
					};

					MovingHeadToWorld();
					void init();
					string getTypeName() const override;

					void serialize(Json::Value &);
					void deserialize(const Json::Value &);
					void populateInspector(ofxCvGui::ElementGroupPtr);

					void drawWorld() override;

					void addCapture();
					void calibrate();
				protected:
					vector<DataPoint> dataPoints;
					float residual;
				};
			}
		}
	}
}
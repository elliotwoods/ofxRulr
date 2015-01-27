#pragma once

#include "../Base.h"
#include "ofxCvMin.h"
#include "ofxOsc.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class ProjectorIntrinsicsExtrinsics : public Base {
			public:
				struct Correspondence {
					ofVec3f world;
					ofVec2f projector;
				};

				ProjectorIntrinsicsExtrinsics();
				void init() override;
				string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void calibrate();
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);

				void addPoint(float x, float y, int projectorWidth, int projectorHeight);

				ofxOscReceiver oscServer;

				vector<Correspondence> correspondences;

				float lastSeenSuccess;
				float lastSeenFail;

				float error;

				ofParameter<bool> fixAspectRatio;
			};
		}
	}
}

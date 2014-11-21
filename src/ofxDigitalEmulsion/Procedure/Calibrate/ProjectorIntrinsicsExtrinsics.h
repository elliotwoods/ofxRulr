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

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

				void calibrate();
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;

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

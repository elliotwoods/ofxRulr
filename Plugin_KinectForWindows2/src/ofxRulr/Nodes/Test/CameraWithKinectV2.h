#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvGui/Panels/Draws.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			class CameraWithKinectV2 : public Nodes::Base {
			public:
				CameraWithKinectV2();
				string getTypeName() const override;

				void init();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void drawWorld();

			protected:
				void populateInspector(ofxCvGui::InspectArguments &);
				
				ofVec2f getUndistorted(const vector<float> & distortion, const ofVec2f & point);
				
				ofFloatPixels depthToWorldTable;

				// 0 = when selected
				// 1 = always
				ofParameter<int> activewhen;
			};
		}
	}
}

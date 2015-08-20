#pragma once

#include "ofxRulr/Nodes/Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class AimMovingHeadAt : public Nodes::Base {
			public:
				AimMovingHeadAt();
				void init();
				string getTypeName() const override;
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				void populateInspector(ofxCvGui::ElementGroupPtr);
			protected:
				ofParameter<bool> ignoreBlankTransform;

				struct {
					ofParameter<bool> enabled;
					ofParameter<float> delay;
				} prediction;
			};
		}
	}
}
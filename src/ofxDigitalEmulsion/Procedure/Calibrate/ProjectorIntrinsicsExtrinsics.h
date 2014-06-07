#pragma once

#include "../Base.h"
#include "ofxCvMin.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class ProjectorIntrinsicsExtrinsics : public Base {
			public:
				ProjectorIntrinsicsExtrinsics();
				string getTypeName() const override;
				Graph::PinSet getInputPins() override;
				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void serialize(Json::Value &);
				void deserialize(Json::Value &);
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				Graph::PinSet inputPins;
			};
		}
	}
}
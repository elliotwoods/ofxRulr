#pragma once

#include "../Base.h"
#include "ofxCvMin.h"
#include "ofxOsc.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class ViewToVertices : public Base {
			public:
				ViewToVertices();
				string getTypeName() const override;
				void init();
				ofxCvGui::PanelPtr getView() override;
				void update();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void calibrate();
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);
				ofxCvGui::PanelPtr view;
			};
		}
	}
}

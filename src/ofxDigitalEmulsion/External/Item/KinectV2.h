#pragma once

#include "../../Item/Base.h"

#include "ofxKinectForWindows2.h"

namespace ofxDigitalEmulsion {
	namespace External {
		namespace Item {
			class KinectV2 : public ofxDigitalEmulsion::Item::Base {
			public:
				KinectV2();
				string getTypeName() const override;
				void update() override;
				ofxCvGui::PanelPtr getView() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

				void drawWorld();
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr);
				shared_ptr<ofxKinectForWindows2::Device> device;
				ofxCvGui::PanelPtr view;
			};
		}
	}
}
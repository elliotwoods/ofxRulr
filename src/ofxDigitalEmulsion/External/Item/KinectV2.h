#pragma once

#include "../../Item/Base.h"

#include "ofxKinectForWindows2.h"

#include "ofxCvGui/Panels/Groups/Grid.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class KinectV2 : public ofxDigitalEmulsion::Item::Base {
		public:
			KinectV2();
			void init() override;
			string getTypeName() const override;
			void update() override;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			void drawWorld();
			shared_ptr<ofxKinectForWindows2::Device> getDevice();

		protected:
			void populateInspector2(ofxCvGui::ElementGroupPtr);
			shared_ptr<ofxKinectForWindows2::Device> device;
			shared_ptr<ofxCvGui::Panels::Groups::Grid> view;

			ofParameter<int> playState;
			ofParameter<int> viewType;
		};
	}
}
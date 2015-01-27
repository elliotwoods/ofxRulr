#pragma once

#include "../../Graph/IRigidBody.h"

#include "ofxKinectForWindows2.h"

#include "ofxCvGui/Panels/Groups/Grid.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class KinectV2 : public ofxDigitalEmulsion::Graph::IRigidBody {
		public:
			KinectV2();
			void init() override;
			string getTypeName() const override;
			void update() override;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &);
			void deserialize(const Json::Value &);

			void drawObject() override;
			shared_ptr<ofxKinectForWindows2::Device> getDevice();

		protected:
			void populateInspector(ofxCvGui::ElementGroupPtr);
			shared_ptr<ofxKinectForWindows2::Device> device;
			shared_ptr<ofxCvGui::Panels::Groups::Grid> view;

			ofParameter<int> playState;
			ofParameter<int> viewType;
		};
	}
}
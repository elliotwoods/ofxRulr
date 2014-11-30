#pragma once

#include "Base.h"

namespace ofxDigitalEmulsion {
	namespace MotionCapture {
		class Vicon : public Base {
		public:
			Vicon();
			string getTypeName() const override;
			void init() override;
			void update();

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;
			
			ofxCvGui::PanelPtr getView() override;
			void populateInspector2(ofxCvGui::ElementGroupPtr) override;

			///MotionCapture
			///{
			shared_ptr<Frame> getCurrentFrame() override;
			///}
		protected:
			ofxCvGui::PanelPtr view;
			shared_ptr<Frame> currentFrame;
		};
	}
}
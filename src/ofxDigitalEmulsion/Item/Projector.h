#pragma once

#include "Base.h"

namespace ofxDigitalEmulsion {
	namespace Item {
		class Projector : public Base {
		public:
			string getTypeName() const;
			ofxCvGui::PanelPtr getView() override;

			void serialize(Json::Value &) override;
			void deserialize(const Json::Value &) override;

			float getWidth();
			float getHeight();
		protected:
			int width, height;
			void populateInspector2(ofxCvGui::ElementGroupPtr);
		};
	}
}
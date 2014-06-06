#pragma once

#include "../Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class CameraIntrinsics : public Base {
			public:
				CameraIntrinsics();
				string getTypeName() const override;
				Graph::PinSet getInputPins() override;
				ofxCvGui::PanelPtr getView() override;
			protected:
				Graph::PinSet inputPins;
			};
		}
	}
}
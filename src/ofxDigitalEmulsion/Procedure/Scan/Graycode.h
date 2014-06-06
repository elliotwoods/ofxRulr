#pragma once

#include "../Base.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Scan {
			class Graycode : public Procedure::Base {
			public:
				Graycode();
				string getTypeName() const override;
				Graph::PinSet getInputPins() override;
			protected:
				Graph::PinSet inputPins;
			};
		}
	}
}
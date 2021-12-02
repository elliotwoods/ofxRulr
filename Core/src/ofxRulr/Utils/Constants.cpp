#include "pch_RulrCore.h"

namespace ofxRulr {
	bool isActive(const ofxCvGui::IInspectable* selectable, const WhenActive& whenActive) {
		switch (whenActive.get()) {
		case WhenActive::Selected:
			if (!selectable->isBeingInspected()) {
				return false;
			}
		case WhenActive::Always:
			return true;
			break;
		case WhenActive::Never:
		default:
			return false;
		}
	}
}
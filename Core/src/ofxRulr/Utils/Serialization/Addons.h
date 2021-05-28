#pragma once

#include "ofxRulr/Utils/Serialization/Native.h"

#include "ofxRay.h"

namespace ofxRulr {
	namespace Utils {
		//--
		// ofxRay
		//--
		//
		DECLARE_SERIALIZE_VAR(ofxRay::Ray);
		DECLARE_SERIALIZE_VAR(ofxRay::Plane);
		DECLARE_SERIALIZE_VAR(ofxRay::Camera)
		//
		//--
	}
}
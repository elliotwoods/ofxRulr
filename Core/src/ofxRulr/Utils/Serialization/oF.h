#pragma once

#include "Native.h"

#include "ofMesh.h"
#include "ofRectangle.h"
#include "ofColor.h"

namespace ofxRulr {
	namespace Utils {
		//--
		// GLM
		//--
		//
		DECLARE_SERIALIZE_VAR(glm::vec2)
		DECLARE_SERIALIZE_VAR(glm::vec3)
		DECLARE_SERIALIZE_VAR(glm::vec4)
		DECLARE_SERIALIZE_VAR(glm::mat3)
		DECLARE_SERIALIZE_VAR(glm::mat4)
		DECLARE_SERIALIZE_VAR(glm::quat)
		//
		//--


		//--
		// OpenCV
		//--
		//
		DECLARE_SERIALIZE_VAR(cv::Point2f)
		DECLARE_SERIALIZE_VAR(cv::Point3f)
		//
		//--

		
		//--
		// openFrameworks
		//--
		//
		DECLARE_SERIALIZE_VAR(ofMesh)
		DECLARE_SERIALIZE_VAR(ofRectangle)
		DECLARE_SERIALIZE_VAR(ofColor)
		DECLARE_SERIALIZE_VAR(ofShortColor)
		DECLARE_SERIALIZE_VAR(ofFloatColor)
		//
		//--

	}
}

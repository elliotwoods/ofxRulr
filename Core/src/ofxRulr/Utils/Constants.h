#pragma once

#include "ofxCvGui/Utils/Enum.h"

#define RULR_GL_VERSION_MAJOR 4
#define RULR_GL_VERSION_MINOR 0

#define RULR_MAKE_ELEMENT_SIMPLE(T) static shared_ptr<T> make() { return shared_ptr<T>(new T()); }
#define RULR_MAKE_ELEMENT_HEADER(T, ...) static shared_ptr<T> make(__VA_ARGS__)
#define RULR_MAKE_ELEMENT_BODY(T, ...) return shared_ptr<T>(new T(__VA_ARGS__));
#define MAKE(T, ...) shared_ptr<T>(new T(__VA_ARGS__))

#ifndef __func__
#define __func__ __FUNCTION__
#endif
#define RULR_WARNING ofLogWarning(string(__func__))
#define RULR_ERROR ofLogError(string(__func__))
#define RULR_FATAL ofLogFatalError(string(__func__))

#ifdef TARGET_WIN32
#	if defined(OFXOFXRULR_API_ENTRY_ENABLE)
#		define OFXRULR_API_ENTRY __declspec(dllexport)
#	elif defined(OFXRULR_IMPORTS_ENABLE)
#		define OFXRULR_API_ENTRY __declspec(dllimport)
#	else
#		define OFXRULR_API_ENTRY
#	endif
#else
#	define OFXRULR_API_ENTRY
#endif

#include "ofxCvGui/InspectController.h"

namespace ofxRulr {
	MAKE_ENUM(WhenActive
		, (Always, Selected, Never)
		, ("Always", "Selected", "Never"));

	MAKE_ENUM(FindBoardMode
		, (Raw, Optimized, Assistant)
		, ("Raw", "Optimized", "Assistant"));

	bool isActive(const ofxCvGui::IInspectable*, const WhenActive&);
}

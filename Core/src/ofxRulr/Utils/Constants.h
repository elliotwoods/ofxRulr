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

#ifdef RULR_EXPORT_LIBRARY
	#define RULR_EXPORTS __declspec(dllexport)
	#define OF_EXPORTS_ENABLED
#else
	#define RULR_EXPORTS
#endif

namespace ofxRulr {
	MAKE_ENUM(WhenDrawWorld
		, (Selected, Always, Never)
		, ("Selected", "Always", "Never"));

	MAKE_ENUM(FindBoardMode
		, (Raw, Optimized, Assistant)
		, ("Raw", "Optimized", "Assistant"));
}

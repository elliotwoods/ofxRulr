#pragma once

#define OFXDIGITALEMULSION_MAKE_ELEMENT_SIMPLE(T) static shared_ptr<T> make() { return shared_ptr<T>(new T()); }
#define OFXDIGITALEMULSION_MAKE_ELEMENT_HEADER(T, ...) static shared_ptr<T> make(__VA_ARGS__)
#define OFXDIGITALEMULSION_MAKE_ELEMENT_BODY(T, ...) return shared_ptr<T>(new T(__VA_ARGS__));
#define MAKE(T, ...) shared_ptr<T>(new T(__VA_ARGS__))

#ifndef __func__
#define __func__ __FUNCTION__
#endif
#define OFXDIGITALEMULSION_WARNING ofLogWarning(string(__func__))
#define OFXDIGITALEMULSION_ERROR ofLogError(string(__func__))
#define OFXDIGITALEMULSION_FATAL ofLogFatalError(string(__func__))
#pragma once

#define OFXDIGITALEMULSION_MAKE_ELEMENT_SIMPLE(T) static shared_ptr<T> make() { return shared_ptr<T>(new T()); }
#define OFXDIGITALEMULSION_MAKE_ELEMENT_HEADER(T, ...) static shared_ptr<T> make(__VA_ARGS__)
#define OFXDIGITALEMULSION_MAKE_ELEMENT_BODY(T, ...) return shared_ptr<T>(new T(__VA_ARGS__));
#include "Exception.h"

namespace ofxRulr {
	//---------
	Exception::Exception(const std::string message) : message(message) {	}

	//---------
	const char * Exception::what() const throw() {
		return this->message.c_str();
	}
}
#pragma once

#include <exception>
#include <string>

namespace ofxDigitalEmulsion {
	namespace Utils {
		class Exception : public std::exception {
		public:
			Exception(const std::string message);
			const char * what() const throw() override;
		protected:
			const std::string message;
		};
	}
}
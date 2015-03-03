#pragma once

#include <exception>
#include <string>
#include "opencv2/core/core.hpp"

namespace ofxDigitalEmulsion {
	class Exception : public std::exception {
	public:
		Exception(const std::string message);
		const char * what() const throw() override;
	protected:
		const std::string message;
	};
}

#define OFXDIGITALEMULSION_CATCH_ALL_TO(X) \
	catch (ofxDigitalEmulsion::Exception e) { X; } \
	catch (cv::Exception e) { X; } \
	catch (std::exception e) { X; }

#define OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT \
	OFXDIGITALEMULSION_CATCH_ALL_TO(ofSystemAlertDialog(e.what()))

#define OFXDIGITALEMULSION_CATCH_ALL_TO_ERROR \
	OFXDIGITALEMULSION_CATCH_ALL_TO(ofLogError("ofxDigitalEmulsion") << e.what())

#pragma once

#include <exception>
#include <string>
#include "opencv2/core/core.hpp"

namespace ofxRulr {
	class Exception : public std::exception {
	public:
		Exception(const std::string message);
		const char * what() const throw() override;
	protected:
		const std::string message;
	};
}

#define RULR_CATCH_ALL_TO(X) \
	catch (ofxRulr::Exception e) { X; } \
	catch (cv::Exception e) { X; } \
	catch (std::exception e) { X; }

#define RULR_CATCH_ALL_TO_ALERT \
	RULR_CATCH_ALL_TO(ofSystemAlertDialog(e.what()))

#define RULR_CATCH_ALL_TO_ERROR \
	RULR_CATCH_ALL_TO(ofLogError("ofxRulr") << e.what())

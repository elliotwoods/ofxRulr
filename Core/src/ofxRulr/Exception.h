#pragma once

#include <exception>
#include <string>
#include "opencv2/core/core.hpp"

#include "ofxRulr/Utils/Constants.h"
#include "ofxMachineVision/Constants.h"

#ifndef __func__
#define __func__ __FUNCTION__
#endif

namespace ofxRulr {
	class OFXRULR_API_ENTRY Exception : public std::exception {
	public:
		Exception(const std::string & errorMessage);
		const char * what() const throw() override;
	protected:
		const std::string errorMessage;
	};

	class OFXRULR_API_ENTRY NotImplementedException : Exception
	{
	public:
		NotImplementedException();
	};

	class OFXRULR_API_ENTRY TracebackException : public Exception {
	public:
		TracebackException(const std::string & errorMessage
			, const std::string & fileName
			, const std::string & functionName
			, const std::size_t lineNumber);
		std::string getTraceback() const;
	protected:
		const std::string fileName;
		const std::string functionName;
		const std::size_t lineNumber;
	};
}

#define RULR_EXCEPTION(X) \
	ofxRulr::TracebackException(X \
		, string(__FILE__ ) \
		, string(__func__) \
		, __LINE__)

#define RULR_CATCH_ALL_TO(X) \
	catch (const ofxRulr::Exception & e) { X; } \
	catch (const cv::Exception & e) { X; } \
	catch (const ofxMachineVision::Exception & e) { X; } \
	catch (const nlohmann::json::exception & e) { X; } \
	catch (const std::exception & e) { X; }

#define RULR_CATCH_ALL_TO_ALERT \
	RULR_CATCH_ALL_TO(ofSystemAlertDialog(e.what()))

#define RULR_CATCH_ALL_TO_ERROR \
	RULR_CATCH_ALL_TO(ofLogError("ofxRulr") << e.what())

#define RULR_CATCH_ALL_TO_WARNING \
	RULR_CATCH_ALL_TO(ofLogWarning("ofxRulr") << e.what())
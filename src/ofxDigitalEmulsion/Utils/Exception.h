#pragma once

#include <exception>
#include <string>
#include "opencv2/core/core.hpp"

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

#define OFXDIGITALEMULSION_CATCH_ALL_TO_ALERT \
	catch(ofxDigitalEmulsion::Utils::Exception e) { ofSystemAlertDialog(e.what()); } \
	catch (cv::Exception e) { ofSystemAlertDialog(e.what()); } \
	catch (std::exception e) { ofSystemAlertDialog(e.what()); }

#define OFXDIGITALEMULSION_CATCH_ALL_TO_ERROR \
	catch (ofxDigitalEmulsion::Utils::Exception e) { ofLogError("ofxDigitalEmulsion") << e.what(); } \
	catch (cv::Exception e) { ofLogError("ofxDigitalEmulsion") << e.what(); } \
	catch (std::exception e) { ofLogError("ofxDigitalEmulsion") << e.what(); }


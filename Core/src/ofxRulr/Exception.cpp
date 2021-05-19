#include "pch_RulrCore.h"
#include "Exception.h"

#include <sstream>

using namespace std;

namespace ofxRulr {
	//---------
	Exception::Exception(const string & errorMessage)
	: errorMessage(errorMessage) {

	}

	//---------
	const char * Exception::what() const throw() {
		return this->errorMessage.c_str();
	}

	//---------
	TracebackException::TracebackException(const string & errorMessage, const string & fileName, const string & functionName, const size_t lineNumber)
		: Exception(errorMessage)
		, fileName(fileName)
		, functionName(functionName)
		, lineNumber(lineNumber) {

	}

	//---------
	string TracebackException::getTraceback() const {
		stringstream ss;
		ss << this->fileName << ":" << this->lineNumber << " - " << this->functionName;
		return ss.str();
	}

	NotImplementedException::NotImplementedException()
		: Exception("Not implemented") {

	}
}
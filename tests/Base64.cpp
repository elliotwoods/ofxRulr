#include "ofxRulr/Utils/Base64.h"
#include <cstdint>
#include <iostream>

int main() {
	using namespace ofxRulr::Utils::Base64;
	const unsigned char hello[] = "hello";
	const uint64_t original = UINT64_C(0xfedcba9876543210);
	uint64_t restored = 0;
	if (encode(hello, 5) != "aGVsbG8=" || decode(std::string("aGVsbG8=")) != "hello"
		|| !decode(encode(original), restored) || restored != original
		|| decode(std::string("aA=="), restored)) {
		std::cerr << "Base64 round trip failed" << std::endl;
		return 1;
	}
	std::cout << "Base64 DLL round trip passed" << std::endl;
	return 0;
}

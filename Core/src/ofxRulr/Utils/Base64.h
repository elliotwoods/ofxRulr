#pragma once

#include <string>

namespace ofxRulr {
	namespace Utils {
		namespace Base64 {
			std::string encode(unsigned char const * plainText, unsigned int length);
			std::string decode(std::string const& encoded);

			template<typename T>
			std::string encode(const T & plainText) {
				return encode((unsigned char *) & plainText, sizeof(T));
			}

			template<typename T>
			bool decode(std::string const & encoded, T & data) {
				auto decodedString = decode(encoded);
				if (sizeof(T) == decodedString.size()) {
					memcpy(&data, &decodedString[0], sizeof(T));
					return true;
				}
				else {
					return false;
				}
			}
		};
	}
}
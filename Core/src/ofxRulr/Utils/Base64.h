#pragma once

#include <string>
#include <cstring>
#include <type_traits>

#if defined(_WIN32) && defined(OFXRULR_BASE64_EXPORTS)
#define OFXRULR_BASE64_API __declspec(dllexport)
#elif defined(_WIN32)
#define OFXRULR_BASE64_API __declspec(dllimport)
#else
#define OFXRULR_BASE64_API
#endif

namespace ofxRulr {
	namespace Utils {
		namespace Base64 {
			OFXRULR_BASE64_API std::string encode(unsigned char const * plainText, unsigned int length);
			OFXRULR_BASE64_API std::string decode(std::string const& encoded);

			template<typename T>
			std::string encode(const T & plainText) {
				static_assert(std::is_trivially_copyable<T>::value, "Base64 requires a trivially copyable type");
				return encode((unsigned char *) & plainText, sizeof(T));
			}

			template<typename T>
			bool decode(std::string const & encoded, T & data) {
				static_assert(std::is_trivially_copyable<T>::value, "Base64 requires a trivially copyable type");
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
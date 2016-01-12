#pragma once

#include <string>
#include <list>

using namespace std;

namespace ofxRulr {
	namespace Data {
		namespace Channels {
			class Address : public list<string> {
			public:
				Address();
				Address(const string & addressString);
			};
		}
	}
}
#include "pch_RulrNodes.h"
#include "Address.h"

namespace ofxRulr {
	namespace Data {
		namespace Channels {
			//----------
			Address::Address() {

			}

			//----------
			Address::Address(const string & addressString) {
				auto addressStringMutable = addressString;

				while (true) {
					//ignore any trailing slashes (i.e. empty last part)
					if (addressStringMutable.empty()) {
						break;
					}

					//look for next seperation
					auto findPath = addressStringMutable.find("/");
					if (findPath == string::npos) {
						//no more path deliminators
						this->push_back(addressStringMutable);
						break;
					}
					else {
						//split it
						auto channelName = addressStringMutable.substr(0, findPath);
						addressStringMutable.erase(0, findPath + 1);
						this->push_back(channelName);
					}
				}
			}
		}
	}
}

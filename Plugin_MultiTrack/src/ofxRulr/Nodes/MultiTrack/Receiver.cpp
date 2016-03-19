#include "pch_MultiTrack.h"
#include "Receiver.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MultiTrack {
			//----------
			Receiver::Receiver() {
				this->addGetFunction<Data::DepthFrame>([this]() {
					return shared_ptr<Data::DepthFrame>();
				});
			}

			//----------
			string Receiver::getTypeName() const {
				return "MultiTrack::Receiver";
			}
		}
	}
}
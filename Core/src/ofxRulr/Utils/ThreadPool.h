#pragma once

#include "ofThreadChannel.h"
#include "ofxRulr/Utils/Constants.h"
#include <thread>
#include <set>

using namespace std;

namespace ofxRulr {
	namespace Utils {
		class ThreadPool {
		public:
			ThreadPool(size_t poolSize, size_t maxQueueSize);
			virtual ~ThreadPool();

			bool performAsync(function<void()>);
		protected:
			set<unique_ptr<thread>> threads;
			ofThreadChannel<std::function<void()>> actionQueue;
			size_t maxQueueSize;

			bool joining = false;
		};
	}
}
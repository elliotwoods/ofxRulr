#pragma once

#include "ofThreadChannel.h"
#include "ofxRulr/Utils/Constants.h"
#include <thread>
#include <set>
#include <future>

using namespace std;

namespace ofxRulr {
	namespace Utils {
		class ThreadPool {
		public:
			ThreadPool(size_t poolSize, size_t maxQueueSize);
			virtual ~ThreadPool();

			bool performAsync(function<void()>);

			//consider using std::async(std::launch::async, ...) instead
			template<typename ReturnType>
			future<exception_ptr> performAsyncWithExceptionHandling(function<void()>) {
				promise<ReturnType> promise;
				auto future = promise.get_future();
				auto wrappedFunction = [function, &promise]() {
					try {
						promise.set_value(function());
					}
					catch (...) {
						promise.set_exception(std::current_exception());
					}
				};
				if (!this->performAsync(function)) {
					try {
						throw(ofxRulr::Exception("Thread pool action queue is full"));
					}
					catch (...) {
						promise.set_exception(std::current_exception());
					}
				}
				return future;
			}
		protected:
			set<unique_ptr<thread>> threads;
			ofThreadChannel<std::function<void()>> actionQueue;
			size_t maxQueueSize;

			bool joining = false;
		};
	}
}
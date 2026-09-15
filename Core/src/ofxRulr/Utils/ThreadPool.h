#pragma once

#include "ofThreadChannel.h"
#include "ofxRulr/Utils/Constants.h"
#include <thread>
#include <set>
#include <future>
#include <atomic>
#include "ofxRulr/Exception.h"

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
			future<ReturnType> performAsyncWithExceptionHandling(function<ReturnType()> action) {
				auto promise = make_shared<std::promise<ReturnType>>();
				auto future = promise->get_future();
				auto wrappedFunction = [action, promise]() {
					try {
						if constexpr (std::is_void_v<ReturnType>) {
							action();
							promise->set_value();
						}
						else {
							promise->set_value(action());
						}
					}
					catch (...) {
						promise->set_exception(std::current_exception());
					}
				};
				if (!this->performAsync(wrappedFunction)) {
					promise->set_exception(make_exception_ptr(ofxRulr::Exception("Thread pool action queue is full")));
				}
				return future;
			}

			size_t getQueueSize() const;
		protected:
			set<unique_ptr<thread>> threads;
			ofThreadChannel<std::function<void()>> actionQueue;
			size_t maxQueueSize;

			atomic<bool> joining{ false };
		};
	}
}
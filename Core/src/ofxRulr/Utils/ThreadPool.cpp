#include "pch_RulrCore.h"
#include "ThreadPool.h"
#include "ofxRulr/Exception.h"
#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		ThreadPool::ThreadPool(size_t poolSize, size_t maxQueueSize)
		: maxQueueSize(maxQueueSize) {
			for (size_t i = 0; i < poolSize; i++) {
				auto thread = make_unique<std::thread>([this]() {
					std::function<void()> action;
					while (!this->joining) {
						if (this->actionQueue.tryReceive(action, 1000)) {
							try {
								action();
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				});
				this->threads.insert(move(thread));
			}
		}

		//----------
		ThreadPool::~ThreadPool() {
			this->actionQueue.close();

			this->joining = true;
			for (auto & thread : this->threads) {
				if (thread->joinable()) {
					thread->join();
				}
			}
		}

		//----------
		bool ThreadPool::performAsync(function<void()> function) {
			if (this->actionQueue.size() >= this->maxQueueSize) {
				return false;
			}

			this->actionQueue.send(move(function));
			return true;
		}
	}
}
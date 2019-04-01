#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/ThreadPool.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			template<class IncomingNodeType
				, class IncomingFrameType
				, class OutgoingFrameType>
				class ThreadedProcessNode : public Nodes::Base {
			private:
				atomic<float> processingTime = 0;
				atomic<int> processedFramesSinceLastAppFrame = 0;
				atomic<int> droppedFramesSinceLastAppFrame = 0;

				float processedFramesPerSecond = 0.0f;
				float droppedFramesPerSecond = 0.0f;
				unique_ptr<Utils::ThreadPool> threadPool;

				struct : ofParameterGroup {
					ofParameter<bool> performInParentThread{ "Perform in parent thread", false };

					struct : ofParameterGroup {
						ofParameter<bool> keepLastFrame{ "Keep last frame", false };
						ofParameter<bool> performEveryAppFrame{ "Perform every app frame", false };
						PARAM_DECLARE("Debug", keepLastFrame, performEveryAppFrame);
					} debug;

					PARAM_DECLARE("ThreadedProcessNode", performInParentThread, debug);
				} parameters;
			protected:
				virtual void processFrame(shared_ptr<IncomingFrameType> incomingFrame) = 0;
				virtual size_t getThreadPoolSize() const { return 2; }
				virtual size_t getThreadPoolQueueSize() const { return 3; }

				// For performEveryAppFrame
				shared_ptr<IncomingFrameType> lastFrame;
				ofxCvGui::ElementPtr reprocessLastFrameButton;

				function<void()> constructAction(shared_ptr<IncomingFrameType> incomingFrame) {
					return [this, incomingFrame]() {
						auto timeStart = chrono::high_resolution_clock::now();
						this->processFrame(incomingFrame);
						chrono::duration<float, ratio<1, 1000>> duration = chrono::high_resolution_clock::now() - timeStart;
						this->processingTime.store(duration.count());
						this->processedFramesSinceLastAppFrame++;
					};
				}
			public:
				ThreadedProcessNode() {
					RULR_NODE_INIT_LISTENER;
				}

				void init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;

					this->threadPool = make_unique<Utils::ThreadPool>(this->getThreadPoolSize(), this->getThreadPoolQueueSize());

					auto input = this->addInput<IncomingNodeType>();
					input->onNewConnection += [this](shared_ptr<IncomingNodeType> inputNode) {
						inputNode->onNewFrame.addListener([this](shared_ptr<IncomingFrameType> incomingFrame) {
							auto action = this->constructAction(incomingFrame);

							if (this->parameters.performInParentThread) {
								action();
							}
							else {
								if (!this->threadPool->performAsync(action)) {
									this->droppedFramesSinceLastAppFrame++;
								}
							}

							if (this->parameters.debug.keepLastFrame) {
								this->lastFrame = incomingFrame;
							}
						}, this);
					};
					input->onDeleteConnection += [this](shared_ptr<IncomingNodeType> inputNode) {
						if (inputNode) {
							inputNode->onNewFrame.removeListeners(this);
						}
					};

					this->manageParameters(this->parameters);
				}

				void update() {
					if (this->reprocessLastFrameButton) {
						if (this->lastFrame) {
							this->reprocessLastFrameButton->setEnabled(true);
						}
						else {
							this->reprocessLastFrameButton->setEnabled(false);
						}
					}

					auto processedFramesPerSecond = (float)processedFramesSinceLastAppFrame.load() / ofGetLastFrameTime();
					this->processedFramesPerSecond = ofLerp(this->processedFramesPerSecond, processedFramesPerSecond, 0.1f);
					this->processedFramesSinceLastAppFrame.store(0);

					auto droppedFramesPerSecond = (float)droppedFramesSinceLastAppFrame.load() / ofGetLastFrameTime();
					this->droppedFramesPerSecond = ofLerp(this->droppedFramesPerSecond, droppedFramesPerSecond, 0.1f);
					this->droppedFramesSinceLastAppFrame.store(0);

					// Perform in app frame
					{
						auto lastFrame = this->lastFrame; // Take a copy so thread can't change
						if (lastFrame) {
							if (this->parameters.debug.performEveryAppFrame) {
								auto action = this->constructAction(lastFrame);
								if (!this->threadPool->performAsync(action)) {
									this->droppedFramesSinceLastAppFrame++;
								}
							}

							if (!this->parameters.debug.keepLastFrame) {
								this->lastFrame.reset();
							}
						}
					}
				}

				void populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addIndicatorBool("Last frame available", [this]() {
						return (bool) this->lastFrame;
					});

					this->reprocessLastFrameButton = inspector->addButton("Reprocess last frame", [this]() {
						auto lastFrame = this->lastFrame;
						if (lastFrame) {
							auto action = this->constructAction(lastFrame);
							if (!this->threadPool->performAsync(action)) {
								this->droppedFramesSinceLastAppFrame++;
							}
						}
					}, ' ');

					inspector->addLiveValueHistory("Processing time [ms]", [this]() {
						return this->processingTime.load();
					});

					inspector->addLiveValueHistory("Queue size", [this]() {
						return this->threadPool->getQueueSize();
					});

					inspector->addLiveValueHistory("Frames processed [Hz]", [this]() {
						return this->processedFramesPerSecond;
					});
					inspector->addLiveValueHistory("Dropped frames [Hz]", [this]() {
						return this->droppedFramesPerSecond;
					});
				}

				//happens in 'our thread'
				ofxLiquidEvent<shared_ptr<OutgoingFrameType>> onNewFrame;
			};
		}
	}
}

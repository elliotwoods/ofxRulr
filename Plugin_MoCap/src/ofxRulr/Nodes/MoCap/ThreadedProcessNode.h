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
				atomic<int> processedFramesSinceLastAppFrame = 0;
				atomic<int> droppedFramesSinceLastAppFrame = 0;
			protected:
				unique_ptr<Utils::ThreadPool> threadPool;
				float processedFramesPerSecond = 0.0f;
				float droppedFramesPerSecond = 0.0f;

				virtual void processFrame(shared_ptr<IncomingFrameType> incomingFrame) = 0;
			public:
				ThreadedProcessNode() {
					RULR_NODE_INIT_LISTENER;
					this->setIcon(Nodes::GraphicsManager::X().getIcon("MoCap::icon"));
				}

				void init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_UPDATE_LISTENER;

					this->threadPool = make_unique<Utils::ThreadPool>(2, 10);

					auto input = this->addInput<IncomingNodeType>();
					input->onNewConnection += [this](shared_ptr<IncomingNodeType> inputNode) {
						inputNode->onNewFrame.addListener([this](shared_ptr<IncomingFrameType> incomingFrame) {
							if (!this->threadPool->performAsync([this, incomingFrame]() {
								this->processFrame(incomingFrame);
								this->processedFramesSinceLastAppFrame++;
							})) {
								this->droppedFramesSinceLastAppFrame++;
							}
						}, this);
					};
					input->onDeleteConnection += [this](shared_ptr<IncomingNodeType> inputNode) {
						if (inputNode) {
							inputNode->onNewFrame.removeListeners(this);
						}
					};
				}

				void update() {
					auto processedFramesPerSecond = (float)processedFramesSinceLastAppFrame.load() / ofGetLastFrameTime();
					this->processedFramesPerSecond = ofLerp(this->processedFramesPerSecond, processedFramesPerSecond, 0.1f);
					this->processedFramesSinceLastAppFrame.store(0);

					auto droppedFramesPerSecond = (float)droppedFramesSinceLastAppFrame.load() / ofGetLastFrameTime();
					this->droppedFramesPerSecond = ofLerp(this->droppedFramesPerSecond, droppedFramesPerSecond, 0.1f);
					this->droppedFramesSinceLastAppFrame.store(0);
				}

				void populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;
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

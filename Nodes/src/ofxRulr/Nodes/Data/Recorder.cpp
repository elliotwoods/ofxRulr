#include "Recorder.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Panels/Scroll.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
#pragma mark public
			//----------
			Recorder::Recorder() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Recorder::getTypeName() const {
				return "Recorder";
			}

			//----------
			void Recorder::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				auto masterRecorderPin = this->addInput<Recorder>("Master");
				masterRecorderPin->onNewConnection += [this](shared_ptr<Recorder> master) {
					master->registerSlave(this->shared_from_this());
					this->flagRebuildView = true;
				};
				masterRecorderPin->onDeleteConnection += [this](shared_ptr<Recorder> master) {
					master->unregisterSlave(this->shared_from_this());
					this->flagRebuildView = true;
				};

				this->view = make_shared<Panels::Scroll>();

				this->trackView = make_shared<Element>();
				this->trackView->setBounds(ofRectangle(0, 0, 100, 50));
				this->trackView->onDraw += [this](DrawArguments & args) {
					ofDrawBitmapString(this->getName(), 10, 20);
					ofDrawBitmapString("Frame count : " + ofToString(this->frames.size()), 10, 30);
				};

				this->state = State::Stopped;
				this->paused = false;
				this->flagRebuildView = false;
			}

			//----------
			void Recorder::update() {
				if (this->flagRebuildView) {
					this->rebuildView();
					this->flagRebuildView = false;
				}

				switch (this->state) {
				case State::Stopped:
					break;
				case State::Playing:
				{
					break;
				}
				case State::Recording:
				{
					this->performOnFamily([](Recorder & recorder) {
						recorder.recordFrame();
					});
					break;
				}
				}
			}

			//----------
			PanelPtr Recorder::getView() {
				return this->view;
			}

			//----------
			ElementPtr Recorder::getTrackView() {
				return this->trackView;
			}

			//----------
			void Recorder::serialize(Json::Value & json) {
				for (auto frame : this->frames) {
					frame.second->serialize(json[ofToString(frame.first)]);
				}
			}

			//----------
			void Recorder::deserialize(const Json::Value & json) {
				this->clear();
				for (auto frameIndexString : json.getMemberNames()) {
					try {
						auto frameIndex = ofToInt64(frameIndexString);
						auto frame = this->deserializeFrame(json[frameIndexString]);
						if (!frame) {
							throw(ofxRulr::Exception("Couldn't load frame [" + frameIndexString + "]"));
						}
						auto frameInserter = FrameInserter(frameIndex, frame);
						this->frames.insert(frameInserter);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void Recorder::populateInspector(ofxCvGui::ElementGroupPtr inspector) {

			}

			//----------
			void Recorder::record() {
				this->recordStartAppTime = ofGetElapsedTimeMillis();
				if (this->frames.empty()) {
					this->recordStartTrackTime = 0;
				}
				else {
					auto lastFrame = this->frames.end();
					lastFrame--;
					this->recordStartTrackTime = lastFrame->first;
				}
				this->state = State::Recording;
				this->paused = false;
			}

			//----------
			void Recorder::play() {
				if (!this->frames.empty()) {
					this->state = State::Playing;
				}
				else {
					this->state = State::Stopped;
				}
				this->paused = false;
			}

			//----------
			void Recorder::stop() {
				this->state = State::Stopped;
				this->paused = false;
			}

			//----------
			void Recorder::pause() {
				if (this->state == State::Playing || this->state == State::Recording) {
					this->paused = true;
				}
			}

			//----------
			void Recorder::clear() {
				this->performOnFamily([](Recorder & recorder) {
					recorder.frames.clear();
				});
			}

			//----------
			const Recorder::State & Recorder::getState() const {
				return this->state;
			}

			//----------
			bool Recorder::getPaused() const {
				return this->paused;
			}

#pragma mark protected
			//----------
			shared_ptr<Recorder::AbstractFrame> Recorder::getNewSourceFrame() {
				ofLogError(this->getTypeName()) << "getNewSourceFrame() is not implemented";
				return shared_ptr<Recorder::AbstractFrame>();
			}

			//----------
			shared_ptr<Recorder::AbstractFrame> Recorder::deserializeFrame(const Json::Value &) const {
				ofLogError(this->getTypeName()) << "deserializeFrame is not implemented";
				return shared_ptr<Recorder::AbstractFrame>();
			}

			//----------
			void Recorder::registerSlave(shared_ptr<Recorder> slave) {
				auto weakSlave = weak_ptr<Recorder>(slave);
				this->slaves.insert(weakSlave);
				this->flagRebuildView = true;
			}

			//----------
			void Recorder::unregisterSlave(shared_ptr<Recorder> slave) {
				auto weakSlave = weak_ptr<Recorder>(slave);
				this->slaves.erase(weakSlave);
				this->flagRebuildView = true;
			}

			//----------
			void Recorder::performOnFamily(function<void(Recorder &)> action) {
				action(*this);
				for (auto slaveWeak : this->slaves) {
					auto slave = slaveWeak.lock();
					if (slave) {
						action(*slave);
					}
				}
			}

			//----------
			void Recorder::rebuildView() {
				this->view->clear();
				
				if (this->getInput<Recorder>("Master")) {
					// we have a master attached, so no transport controls
					this->view->add(Widgets::Title::make("Use Master to control", Widgets::Title::Level::H3));
				}
				else {
					this->view->add(Widgets::Indicator::make("Playing", [this]() {
						if (this->getState() == State::Playing) {
							return this->getPaused() ? Widgets::Indicator::Status::Warning : Widgets::Indicator::Status::Good;
						}
						return Widgets::Indicator::Status::Clear;
					}));
					this->view->add(Widgets::Indicator::make("Recording", [this]() {
						if (this->getState() == State::Recording) {
							return this->getPaused() ? Widgets::Indicator::Status::Warning : Widgets::Indicator::Status::Good;
						}
						return Widgets::Indicator::Status::Clear;
					}));

					this->view->add(Widgets::Toggle::make("Record", [this]() {
						return this->getState() == State::Recording;
					}, [this](bool record) {
						if (record) {
							this->record();
						}
						else {
							this->stop();
						}
					}));
					this->view->add(Widgets::Toggle::make("Play", [this]() {
						return this->getState() == State::Playing;
					}, [this](bool play) {
						if (play) {
							this->play();
						}
						else {
							this->stop();
						}
					}));
					this->view->add(Widgets::Button::make("Clear", [this]() {
						this->clear();
					}));

					//add our track and child tracks
					this->performOnFamily([this](Recorder & recorder) {
						this->view->add(recorder.getTrackView());
					});
				}
			}

			//----------
			void Recorder::recordFrame() {
				//get the current frame, if it's not empty then store it
				auto currentFrame = this->getNewSourceFrame();
				if (currentFrame) {
					auto recordTrackTime = ofGetElapsedTimeMillis() - recordStartAppTime + recordStartTrackTime;
					const auto inserter = FrameInserter(recordTrackTime, currentFrame);
					this->frames.insert(inserter);
				}
			}
		}
	}
}

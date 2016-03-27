#include "pch_RulrNodes.h"
#include "Recorder.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Panels/Scroll.h"

#include "ofAppRunner.h"

using namespace ofxCvGui;
using namespace std::chrono;

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
#pragma mark public
			//----------
			chrono::microseconds Recorder::getAppTime() {
				return chrono::microseconds(ofGetElapsedTimeMicros());
			}

			//----------
			chrono::microseconds Recorder::getLastAppFrameDuration() {
				auto countMicroseconds = (uint64_t)(ofGetLastFrameTime() * 1e6);
				return microseconds(countMicroseconds);
			}

			//----------
#define stripMagnitude(remainingTime, magnitude, result, magnitudeString) \
	if(remainingTime > magnitude(1)) { \
		auto remainingTimeAtMagnitude = duration_cast<magnitude>(remainingTime); \
		result << remainingTimeAtMagnitude.count() << magnitudeString; \
		remainingTime -= remainingTimeAtMagnitude; \
	}
			//
			string Recorder::formatTime(const microseconds & microseconds) {
				stringstream result;
				if (microseconds < chrono::milliseconds(1)) {
					//if less than a microsecond
					result << microseconds.count() << "us";
				}
				else {
					auto remainingTime = microseconds;
					stripMagnitude(remainingTime, hours, result, "h ");
					result << setw(2) << setfill('0');
					stripMagnitude(remainingTime, minutes, result, "m ");
					stripMagnitude(remainingTime, seconds, result, "s ");

					if (remainingTime > chrono::milliseconds(1)) {
						result << setw(3);
						result << duration_cast<milliseconds>(remainingTime).count() << "ms";
					}
				}
				return result.str();
			}

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
				RULR_NODE_INSPECTOR_LISTENER;

				auto masterRecorderPin = this->addInput<Recorder>("Master");
				masterRecorderPin->onNewConnection += [this](shared_ptr<Recorder> master) {
					master->registerSlave(this);
					this->flagRebuildView = true;
					this->performOnFamily([](Recorder * recorder) {
						recorder->stop(); // always stop all recorders if a sync network is changed
					});
				};
				masterRecorderPin->onDeleteConnection += [this](shared_ptr<Recorder> master) {
					master->unregisterSlave(this);
					this->performOnFamily([](Recorder * recorder) {
						recorder->stop(); // always stop all recorders if a sync network is changed
					});
					this->flagRebuildView = true;
				};

				this->view = make_shared<Panels::Scroll>();

				this->trackView = make_shared<Element>();
				this->trackView->setBounds(ofRectangle(0, 0, 100, 70));
				this->trackView->onDraw += [this](DrawArguments & args) {
					ofDrawBitmapString(this->getName(), 10, 20);
					ofDrawBitmapString("Frame count : " + ofToString(this->frames.size()), 10, 30);
					ofDrawBitmapString("Duration : " + Recorder::formatTime(this->getDuration()), 10, 40);
					ofDrawBitmapString("First frame : " + Recorder::formatTime(this->getFirstFrameTime()), 10, 50);
					ofDrawBitmapString("Playback head : " + Recorder::formatTime(this->getPlaybackHeadPosition()), 10, 60);
				};

				this->loopPlayback.set("Loop", true);

				this->flagRebuildView = false;

				this->stop();
			}

			//----------
			void Recorder::update() {
				if (this->flagRebuildView) {
					this->rebuildView();
					this->flagRebuildView = false;
				}

				switch (this->state) {
				case State::Stopped:
					this->currentFrame = this->getNewSourceFrame();
					break;
				case State::Playing:
				{
					this->playHeadPosition += Recorder::getLastAppFrameDuration();
					if (this->playHeadPosition > this->getLastFrameTime()) {
						if (this->loopPlayback) {
							this->playHeadPosition = chrono::microseconds(0);
						}
						else {
							this->playHeadPosition = this->getLastFrameTime();
							this->stop();
						}
					}
					if (this->state == State::Playing) {
						// if we're still playing, haven't stopped because end of track
						this->currentFrame = this->getFrameAtTime(this->playHeadPosition);
					}
					else {
						this->currentFrame = this->getNewSourceFrame();
					}
					break;
				}
				case State::Recording:
				{
					this->currentFrame = this->getNewSourceFrame();
					this->recordFrame();
					break;
				}
				}
			}

			//----------
			PanelPtr Recorder::getPanel() {
				return this->view;
			}

			//----------
			ElementPtr Recorder::getTrackView() {
				return this->trackView;
			}

			//----------
			void Recorder::serialize(Json::Value & json) {
				auto & jsonFrames = json["frames"];
				for (auto frame : this->frames) {
					frame.second->serialize(jsonFrames[ofToString(frame.first.count())]);
				}
			}

			//----------
			void Recorder::deserialize(const Json::Value & json) {
				this->clear();
				const auto & jsonFrames = json["frames"];
				for (auto frameTimeString : jsonFrames.getMemberNames()) {
					try {
						auto frameTime = ofToInt64(frameTimeString);
						auto frame = this->deserializeFrame(jsonFrames[frameTimeString]);
						if (!frame) {
							throw(ofxRulr::Exception("Couldn't load frame [" + frameTimeString + "]"));
						}
						auto frameTimeMicroseconds = chrono::microseconds(frameTime);
						auto frameInserter = FrameInserter(frameTimeMicroseconds, frame);
						this->frames.insert(frameInserter);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void Recorder::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Toggle(this->loopPlayback));
				inspector->add(new Widgets::Button("Erase blank before first frame", [this]() {
					try {
						this->performOnFamily([](Recorder * recorder) {
							recorder->eraseBlankBeforeFirstFrame();
						});
					}
					RULR_CATCH_ALL_TO_ALERT;
				}));
				inspector->add(new Widgets::Button("Stretch recording by factor", [this]() {
					auto factorString = ofSystemTextBoxDialog("Stretch factor [1x]");
					auto factor = ofToFloat(factorString);
					try {
						this->performOnFamily([factor](Recorder * recorder) {
							recorder->stretchDurationByFactor((double) factor);
						});
					}
					RULR_CATCH_ALL_TO_ALERT;
				}));
			}

			//----------
			void Recorder::record() {
				if (this->frames.empty()) {
					this->recordStartTrackTime = Recorder::getAppTime();
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
				this->playHeadPosition = chrono::microseconds(0);
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
				this->frames.clear();
			}

			//----------
			shared_ptr<Recorder::AbstractFrame> Recorder::getCurrentFrame() const {
				return this->currentFrame;
			}

			//----------
			shared_ptr<Recorder::AbstractFrame> Recorder::getFrameAtTime(const microseconds & time) const {
				auto findFrame = this->frames.lower_bound(time);
				if (findFrame != this->frames.end()) {
					return findFrame->second;
				}
				else {
					return shared_ptr<Recorder::AbstractFrame>();
				}
			}

			//----------
			const Recorder::State & Recorder::getState() const {
				return this->state;
			}

			//----------
			bool Recorder::getPaused() const {
				return this->paused;
			}

			//----------
			size_t Recorder::getFrameCount() const {
				return this->frames.size();
			}

			//----------
			bool Recorder::empty() const {
				return this->frames.empty();
			}

			//----------
			microseconds Recorder::getFirstFrameTime() const {
				if (this->frames.empty()) {
					// we return start and end as being at 0 in this case
					return chrono::microseconds(0);
				}
				else {
					return this->frames.begin()->first;
				}
			}

			//----------
			microseconds Recorder::getLastFrameTime() const {
				if (this->frames.empty()) {
					// we return start and end as being at 0 in this case
					return chrono::microseconds(0);
				}
				else {
					auto lastFrame = this->frames.end();
					lastFrame--;
					return lastFrame->first;
				}
			}

			//----------
			microseconds Recorder::getPlaybackHeadPosition() const {
				return this->playHeadPosition;
			}

			//----------
			void Recorder::erase(chrono::microseconds start, chrono::microseconds end) {
				//delete all frames with timestamp >= start && timestamp < end
				{
					auto startIt = this->frames.lower_bound(start);
					auto endIt = this->frames.lower_bound(end); //stop when = to end
					this->frames.erase(startIt, endIt);
				}
				
				//also move the timestamp of all frames after end back by (end - start)
				{
					auto eraseDuration = end - start;
					auto it = this->frames.lower_bound(end);
					while (it != this->frames.end()) {
						auto newTimestamp = it->first - eraseDuration;
						swap(this->frames[newTimestamp], it->second);
						this->frames.erase(it++);
					}
				}
			}

			//----------
			void Recorder::eraseBlankBeforeFirstFrame() {
				this->erase(chrono::microseconds(0), this->getFirstFrameTime());
			}

			//----------
			void Recorder::stretchDuration(microseconds newDuration) {
				auto factor = (double)newDuration.count() / (double) this->getDuration().count();
				this->stretchDurationByFactor(factor);
			}

			//----------
			void Recorder::stretchDurationByFactor(double factor) {
				if (factor >= 0.0f) {
				}
				else {
					//could be NaN, 0 or negative
					stringstream errorMessage;
					errorMessage << "Recorder : Cannot stretch by a factor of " << factor;
					throw(ofxRulr::Exception(errorMessage.str()));
				}
				//completely reallocate frame holders
				Frames newFrames;

				for (const auto & frame : this->frames) {
					auto newFrameTimeRaw = (double)frame.first.count() * factor;
					microseconds newFrameTime((uint64_t)newFrameTimeRaw);
					auto inserter = FrameInserter(newFrameTime, frame.second);
					newFrames.insert(inserter);
				}

				swap(this->frames, newFrames);
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
			void Recorder::registerSlave(Recorder * slave) {
				this->slaves.insert(slave);
				this->flagRebuildView = true;
			}

			//----------
			void Recorder::unregisterSlave(Recorder * slave) {
				this->slaves.erase(slave);
				this->flagRebuildView = true;
			}

			//----------
			void Recorder::performOnFamily(function<void(Recorder *)> action) {
				action(this);
				if (!this->slaves.empty()) {
					for (auto slave : this->slaves) {
						action(slave);
					}
				}
			}

			//----------
			void Recorder::rebuildView() {
				this->view->clear();
				
				if (this->getInput<Recorder>("Master")) {
					// we have a master attached, so no transport controls
					this->view->add(new Widgets::Title("Use Master to control", Widgets::Title::Level::H3));
				}
				else {
					this->view->add(new Widgets::Indicator("Playing", [this]() {
						if (this->getState() == State::Playing) {
							return this->getPaused() ? Widgets::Indicator::Status::Warning : Widgets::Indicator::Status::Good;
						}
						return Widgets::Indicator::Status::Clear;
					}));
					this->view->add(new Widgets::Indicator("Recording", [this]() {
						if (this->getState() == State::Recording) {
							return this->getPaused() ? Widgets::Indicator::Status::Warning : Widgets::Indicator::Status::Good;
						}
						return Widgets::Indicator::Status::Clear;
					}));

					this->view->add(new Widgets::Toggle("Play", [this]() {
						return this->getState() == State::Playing;
					}, [this](bool play) {
						if (play) {
							this->performOnFamily([](Recorder * recorder) {
								recorder->play();
							});
						}
						else {
							this->performOnFamily([](Recorder * recorder) {
								recorder->stop();
							});
						}
					}));
					this->view->add(new Widgets::Toggle("Record", [this]() {
						return this->getState() == State::Recording;
					}, [this](bool record) {
						if (record) {
							this->performOnFamily([](Recorder * recorder) {
								recorder->record();
							});
						}
						else {
							this->performOnFamily([](Recorder * recorder) {
								recorder->stop();
							});
						}
					}));
					this->view->add(new Widgets::Button("Clear", [this]() {
						this->performOnFamily([](Recorder * recorder) {
							recorder->clear();
						});
					}));

					//add our track and child tracks
					this->performOnFamily([this](Recorder * recorder) {
						this->view->add(recorder->getTrackView());
					});
				}
			}

			//----------
			void Recorder::recordFrame() {
				//if our current frame isn't blank then store it
				if (this->currentFrame) {
					auto recordTrackTime = Recorder::getAppTime() - recordStartAppTime + recordStartTrackTime;
					const auto inserter = FrameInserter(recordTrackTime, this->currentFrame);
					this->frames.insert(inserter);
				}
			}
		}
	}
}

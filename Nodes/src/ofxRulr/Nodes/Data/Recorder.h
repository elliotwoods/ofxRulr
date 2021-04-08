#pragma once

#include "ofxRulr/Utils/Serializable.h"
#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Scroll.h"

#include <chrono>

namespace ofxRulr {
	namespace  Nodes {
		namespace Data {
			/**
			Abstract node to record and playback a data type.
			Your logger class:
			* Implement getTypeName()
			* Implement getNewSourceFrame()
			* Implement deserializeFrame(const nlohmann::json &)
			* Probably inherit another class which provides data of your type to output the recording
			**/
			class Recorder : virtual public ofxRulr::Nodes::Base {
			public:
				typedef ofxRulr::Utils::Serializable AbstractFrame;
				typedef map<chrono::microseconds, shared_ptr<AbstractFrame>> Frames;
				typedef pair<chrono::microseconds, shared_ptr<AbstractFrame>> FrameInserter;

				enum State {
					Stopped, // allow data to flow through
					Playing,
					Recording
				};

				static chrono::microseconds getAppTime();
				static chrono::microseconds getLastAppFrameDuration();

				static string formatTime(const chrono::microseconds &);

				Recorder();
				virtual string getTypeName() const override;
				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				ofxCvGui::ElementPtr getTrackView();

				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				void populateInspector(ofxCvGui::InspectArguments &);

				void record();
				void play();
				void stop();
				void pause(); //pause a playing or recording. doesn't change state
				void clear();

				shared_ptr<AbstractFrame> getCurrentFrame() const;
				shared_ptr<AbstractFrame> getFrameAtTime(const chrono::microseconds &) const;

				const State & getState() const;
				bool getPaused() const;

				size_t getFrameCount() const;
				bool empty() const;
				chrono::microseconds getFirstFrameTime() const;
				chrono::microseconds getLastFrameTime() const;
				chrono::microseconds getDuration() const {
					return this->getLastFrameTime();
				}
				chrono::microseconds getPlaybackHeadPosition() const;

				void erase(chrono::microseconds start, chrono::microseconds end);
				void eraseBlankBeforeFirstFrame();

				void stretchDuration(chrono::microseconds);
				void stretchDurationByFactor(double factor);
			protected:
				//returns an empty pointer if no new data is available this frame
				virtual shared_ptr<AbstractFrame> getNewSourceFrame();

				//returns an empty pointer if can't deserialize
				virtual shared_ptr<AbstractFrame> deserializeFrame(const nlohmann::json &) const;

				void registerSlave(Recorder *);
				void unregisterSlave(Recorder *);
				void performOnFamily(function<void(Recorder *)>);

				void rebuildView();

				void recordFrame();

				State state;
				Frames frames;
				chrono::microseconds recordStartAppTime;
				chrono::microseconds recordStartTrackTime;
				bool paused;
				chrono::microseconds playHeadPosition;
				bool flagRebuildView;

				ofParameter<bool> loopPlayback;

				shared_ptr<AbstractFrame> currentFrame;
				set<Recorder *> slaves;

				shared_ptr<ofxCvGui::Panels::Scroll> view;
				ofxCvGui::ElementPtr trackView;
			};

			/**
			Specialist logger for dealing with 'dumb' data types.
			Your logger class:
			* Implement getNewSourceData()
			* Implement getTypeName()
			* Probably inherit another class which provides data of your type
			*/
			template<typename DataType>
			class StructRecorder : public Recorder {
			public:
				class Frame : public AbstractFrame {
				public:
					///----------
					Frame() {
						this->onSerialize += [this](nlohmann::json & json) {
							json["data64"] = this->encodedData;
						};
						this->onDeserialize += [this](const nlohmann::json & json) {
							if (json["data64"].isString()) {
								this->encodedData = json["data64"].asString();
							}
						};
					}

					//----------
					Frame(const DataType & data) : 
					Frame() {
						this->setInstance(data);
					}

					//----------
					string getTypeName() const override {
						return string(typeid(DataType).name()) + "Frame";
					}
					/*
					//----------
					void setInstance(const DataType & instance) {
						this->encodedData = Utils::Base64::encode(instance);
					}

					//----------
					bool getInstance(DataType & instance) {
						return Utils::Base64::decode(this->encodedData, instance);
					}
					*/
				protected:
					string encodedData;
				};
			protected:
				virtual shared_ptr<DataType> getNewSourceData() = 0;

				shared_ptr<AbstractFrame> getNewSourceFrame() override {
					auto data = this->getNewSourceData();
					if (data) {
						//get encoded data
						auto frame = make_shared<Frame>();
						frame->setInstance(*data);
						return frame;
					}
					else {
						//no data return empty frame
						return shared_ptr<AbstractFrame>();
					}
				}

				shared_ptr<AbstractFrame> deserializeFrame(const nlohmann::json & json) const override {
					auto frame = make_shared<Frame>();
					frame->deserialize(json);
					return frame;
				}
			};
		}
	}
}
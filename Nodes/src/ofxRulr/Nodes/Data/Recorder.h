#pragma once

#include "ofxRulr/Utils/Serializable.h"
#include "ofxRulr/Utils/Base64.h"
#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Scroll.h"

namespace ofxRulr {
	namespace  Nodes {
		namespace Data {
			/**
			Abstract node to record and playback a data type.
			Your logger class:
			* Implement getTypeName()
			* Implement getNewSourceFrame()
			* Implement deserializeFrame(const Json::Value &)
			* Probably inherit another class which provides data of your type to output the recording
			**/
			class Recorder : public ofxRulr::Nodes::Base, public std::enable_shared_from_this<Recorder> {
			public:
				typedef ofxRulr::Utils::Serializable AbstractFrame;
				typedef map<uint64_t, shared_ptr<AbstractFrame>> Frames;
				typedef pair<uint64_t, shared_ptr<AbstractFrame>> FrameInserter;

				enum State {
					Stopped, // allow data to flow through
					Playing,
					Recording
				};

				Recorder();
				virtual string getTypeName() const override;
				void init();
				void update();

				ofxCvGui::PanelPtr getView() override;
				ofxCvGui::ElementPtr getTrackView();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void populateInspector(ofxCvGui::ElementGroupPtr);

				void record();
				void play();
				void stop();
				void pause(); //pause a playing or recording. doesn't change state
				
				void clear();

				const State & getState() const;
				bool getPaused() const;

			protected:
				//returns an empty pointer if no new data is available this frame
				virtual shared_ptr<AbstractFrame> getNewSourceFrame();

				//returns an empty pointer if can't deserialize
				virtual shared_ptr<AbstractFrame> deserializeFrame(const Json::Value &) const;

				void registerSlave(shared_ptr<Recorder>);
				void unregisterSlave(shared_ptr<Recorder>);
				void performOnFamily(function<void(Recorder &)>);

				void rebuildView();

				void recordFrame();

				State state;
				Frames frames;
				uint64_t recordStartAppTime;
				uint64_t recordStartTrackTime;
				bool paused;
				bool flagRebuildView;

				set<weak_ptr<Recorder>, owner_less<weak_ptr<Recorder>>> slaves;

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
						this->onSerialize += [this](Json::Value & json) {
							json["data64"] = this->encodedData;
						};
						this->onDeserialize += [this](const Json::Value & json) {
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

					//----------
					void setInstance(const DataType & instance) {
						this->encodedData = Utils::Base64::encode(instance);
					}

					//----------
					bool getInstance(DataType & instance) {
						return Utils::Base64::decode(this->encodedData, instance);
					}
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

				shared_ptr<AbstractFrame> deserializeFrame(const Json::Value & json) const override {
					auto frame = make_shared<Frame>();
					frame->deserialize(json);
					return frame;
				}
			};
		}
	}
}
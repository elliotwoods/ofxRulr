#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvGui/Panels/Scroll.h"

#include "Base.h"

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
			class Transmit : public DMX::Base {
			public:
				class Universe {
				public:
					Universe();
					void setChannel(ChannelIndex channel, Value value);
					void setChannels(ChannelIndex channelOffset, Value * values, ChannelIndex count);
					const Value * getChannels() const;
					const ofTexture & getTextureReference();
				protected:
					Value values[513]; // 0th channel is unused
					ofTexture preview;
					bool previewDirty;
				};

				Transmit();
				void init();
				void update();
				virtual string getTypeName() const override;
				ofxCvGui::PanelPtr getView() override;

				UniverseIndex getUniverseCount() const;
				const vector<shared_ptr<Universe>> & getUniverses() const;
				shared_ptr<Universe> getUniverse(UniverseIndex universeIndex) const;
			protected:
				void setUniverseCount(UniverseIndex);
				virtual void sendUniverse(UniverseIndex, shared_ptr<Universe>) { }
				shared_ptr<ofxCvGui::Panels::Scroll> view;

				vector<shared_ptr<Universe>> universes;

				bool firstFrame;
			};
		}
	}
}
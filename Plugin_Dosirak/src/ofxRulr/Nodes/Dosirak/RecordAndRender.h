#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxOsc.h"
#include "ofxRulr/Data/Dosirak/Curve.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			class RecordAndRender : public Nodes::Base {
			public:
				MAKE_ENUM(State
					, (Pause, Play, Record)
					, ("Pause", "Play", "Record"));

				typedef vector<glm::vec2> Picture;

				struct Frame {
					Data::Dosirak::Curves curves;
					map<int, Picture> renderedPictures;
				};

				RecordAndRender();
				string getTypeName() const override;
				void init();
				void update();

				ofxCvGui::PanelPtr getPanel() override;
				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&) const;
				void deserialize(const nlohmann::json&);

				void recordCurves(const Data::Dosirak::Curves&);
				void render();

				void play();
				void clear();

				ofxLiquidEvent<void> onNewCurves;

			protected:
				void callbackNewCurves(const Data::Dosirak::Curves&);

				ofParameter<State> state{ "State", State::Pause };
				ofParameter<int> playPosition { "Play position", 0 };

				struct Parameters : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> ignoreDuplicates{ "Ignore duplicates", true };
						PARAM_DECLARE("Recording", ignoreDuplicates);
					} recording;

					struct : ofParameterGroup {
						ofParameter<bool> drawPictures{ "Draw pictures", true };
						PARAM_DECLARE("Rendering", drawPictures);
					} rendering;

					PARAM_DECLARE("RecordAndRender", recording, rendering);
				} parameters;

				ofxCvGui::PanelPtr panel;
				vector<shared_ptr<Frame>> frames;
			};
		}
	}
}
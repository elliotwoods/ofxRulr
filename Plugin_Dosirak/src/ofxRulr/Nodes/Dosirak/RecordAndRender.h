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

					void serialize(nlohmann::json&) const;
					void deserialize(const nlohmann::json&);

					shared_ptr<Frame> clone();
				};

				struct Frames : vector<shared_ptr<Frame>> {
					void serialize(nlohmann::json&) const;
					void deserialize(const nlohmann::json&);
					Frames clone();
				};

				struct FrameSets : map<string, shared_ptr<Frames>> {
					void serialize(nlohmann::json&) const;
					void deserialize(const nlohmann::json&);
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
				void clearRenders();

				void store(string name = "");
				void recall(string name = "");
				vector<string> getStoredRecordingNames() const;
				void clearFrameSets();

				void transportGotoFirst();
				void transportGotoLast();
				void transportGotoNext();
				void transportGotoPrevious();

				void presentCurrentFrame();

				ofxLiquidEvent<void> onNewCurves;

			protected:
				void refreshPanel();
				void callbackNewCurves(const Data::Dosirak::Curves&);

				ofParameter<State> state{ "State", State::Pause };
				ofParameter<int> playPosition { "Play position", 0 };

				struct Parameters : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<bool> ignoreDuplicates{ "Ignore duplicates", true };
						PARAM_DECLARE("Recording", ignoreDuplicates);
					} recording;

					struct : ofParameterGroup {
						ofParameter<bool> useFastMethod{ "Use fast method", true };
						ofParameter<bool> prunePointsOutsideRange{ "Prune points outside of range", true };
						ofParameter<bool> drawPictures{ "Draw pictures", true };
						ofParameter<bool> playAfterRender{ "Play after render", true };
						PARAM_DECLARE("Rendering", useFastMethod, drawPictures, prunePointsOutsideRange, playAfterRender);
					} rendering;

					PARAM_DECLARE("RecordAndRender", recording, rendering);
				} parameters;

				shared_ptr<ofxCvGui::Panels::Widgets> panel;
				Frames frames;
				FrameSets frameSets;
			};
		}
	}
}
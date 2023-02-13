#pragma once

#include "ofxRulr.h"
#include "Laser.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class ExportPictures : public Nodes::Base
			{
			public:
				MAKE_ENUM(PlaybackStyle
					, (Linear, Sine)
					, ("Linear", "Sine"));


				ExportPictures();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);

				void serialize(nlohmann::json&) const;
				void deserialize(const nlohmann::json&);

				void exportPictures(std::filesystem::path& outputFolder) const;

				void addKeyframe();
				void removeKeyframe(size_t index);
				void updateKeyframes();

				struct Frame : ofParameterGroup {
					ofParameter<glm::vec3> position{ "Position", {0, -20, 0} };
					ofParameter<float> diameter{ "Diameter", 2.0f, 0.0f, 10.0f };
					ofParameter<float> brightness{ "Brightness", 1.0f };
					PARAM_DECLARE("", position, diameter, brightness);
				};

				Frame renderFrame(float pct) const;

				void drawFrame(float pct) const;
				void drawFrame(const Frame&) const;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<PlaybackStyle> playbackStyle{ "Playback style", PlaybackStyle::Sine };
						ofParameter<bool> loop{ "Loop", true };
						ofParameter<int> frames{ "Frames", 100 };
						ofParameter<float> duration{ "Duration [s]", 120.0f };
						PARAM_DECLARE("Timeline", playbackStyle, loop, frames, duration);
					} timeline;

					struct : ofParameterGroup {
						ofParameter<bool> enabled{ "Enabled", false };
						ofParameter<float> position{ "Position", 0, 0, 1 };
						PARAM_DECLARE("Preview", enabled, position);
					} preview;

					PARAM_DECLARE("ExportPictures", timeline, preview);
				} parameters;

				vector<shared_ptr<Frame>> keyframes;
			};
		}
	}
}
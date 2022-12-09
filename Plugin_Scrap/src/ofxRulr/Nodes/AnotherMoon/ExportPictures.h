#pragma once

#include "ofxRulr.h"
#include "Laser.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class ExportPictures : public Nodes::Base
			{
			public:
				ExportPictures();
				string getTypeName() const override;

				void init();
				void populateInspector(ofxCvGui::InspectArguments&);

				void exportPictures(std::filesystem::path& outputFolder) const;

				struct MoonParameters : ofParameterGroup {
					ofParameter<glm::vec3> position{ "Position", {0, -20, 0} };
					ofParameter<float> diameter{ "Diameter", 2.0f, 0.0f, 10.0f };
					PARAM_DECLARE("MoonParameters", position, diameter);
					
					MoonParameters(const string& name) {
						this->setName(name);
					}
				};

				struct : ofParameterGroup {
					MoonParameters start{ "Start" };
					MoonParameters end{ "End" };
					PARAM_DECLARE("ExportPictures", start, end);
				} parameters;
			};
		}
	}
}
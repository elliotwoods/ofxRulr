#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include <aruco/aruco.h>
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxRulr/Solvers/MarkerProjections.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			class PLUGIN_ARUCO_EXPORTS Transform : public Nodes::Base {
			public:
				Transform();
				string getTypeName() const override;
				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;

				void loadPhoto();
				void navigate();
				void updatePreview();
				void bakeTransform();

				void drawWorldStage();

				glm::mat4 getTransform() const;
			protected:
				struct : ofParameterGroup {
					ofParameter<WhenActive> autoUpdate{ "Auto update", WhenActive::Selected };

					struct : ofParameterGroup {
						ofParameter<float> x{ "X", 0., -0.2, 0.2 };
						ofParameter<float> y{ "Y", 0., -0.2, 0.2 };
						ofParameter<float> z{ "Z", 0., -0.2, 0.2 };
						PARAM_DECLARE("Translation", x, y, z);
					} translation;

					struct : ofParameterGroup {
						ofParameter<float> x{ "X", 0., -5, 5 };
						ofParameter<float> y{ "Y", 0., -5, 5 };
						ofParameter<float> z{ "Z", 0., -5, 5 };
						PARAM_DECLARE("Rotation", x, y, z);
					} rotation;

					struct : ofParameterGroup {
						ofParameter<float> x{ "X", 1., 0.5, 1.5 };
						ofParameter<float> y{ "Y", 1., 0.5, 1.5 };
						ofParameter<float> z{ "Z", 1., 0.5, 1.5 };
						PARAM_DECLARE("Scale", x, y, z);
					} scale;

					ofParameter<string> filename{ "Filename", "" };

					PARAM_DECLARE("Transform", autoUpdate, translation, rotation, scale, filename);
				} parameters;

				ofFbo fbo;
				ofPixels fboReadBack;
				ofImage photo;
				ofImage preview;
				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
#pragma once

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			struct Pass {
				enum Level : uint8_t {
					None = 0,
					Color,
					Depth,
					DepthPreview,
					Accumulate,
					Availability,
					Done = Availability,
					End
				};
				
				Pass();
				Pass(ofFbo::Settings settings);
				shared_ptr<ofFbo> fbo;
				ofFbo::Settings settings;
				uint64_t renderedFrameIndex = numeric_limits<uint64_t>::max();

				static string toString(Level);
			};
			typedef map<Pass::Level, Pass> Passes;

			//Brightness Availability Projection
			class Projector : public ofxRulr::Nodes::Base {
			public:
				Projector();
				string getTypeName() const override;

				void init();
				void update();
				ofxCvGui::PanelPtr getPanel() override;
				void renderPasses(Pass::Level requiredPassLevel = Pass::Level::Done);
				const Passes & getPasses(Pass::Level requiredPassLevel);
				Pass & getPass(Pass::Level passLevel, bool ensureRendered = true);
				void render(Pass::Level requiredPass);
			protected:

				ofxCvGui::PanelGroupPtr panelGroup;

				Passes passes;

				struct : ofParameterGroup {
					ofParameter<bool> autoRender{ "Auto render", true };
					PARAM_DECLARE("Projector", autoRender);
				} parameters;
			};
		}
	}
}
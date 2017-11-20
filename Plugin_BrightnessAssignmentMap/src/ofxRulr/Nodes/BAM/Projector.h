#pragma once

#include "ofxRulr.h"
#include "Projector.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			struct Pass {
				enum Level : uint8_t {
					None = 0,
					Color,
					Depth,
					DepthPreview,
					AvailabilityProjection,
					AccumulateAvailability,
					BrightnessAssignmentMap,
					Done = BrightnessAssignmentMap,
					End
				};
				
				Pass();
				Pass(ofFbo::Settings settings);

				cv::Mat getHistogram(cv::Mat existingHistogram = cv::Mat());

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
				void populateInspector(ofxCvGui::InspectArguments &);
				const Passes & getPasses(Pass::Level requiredPassLevel);
				Pass & getPass(Pass::Level passLevel, bool ensureRendered = true);
				void render(Pass::Level requiredPass);

				float getBrightness() const;

				void exportPass(Pass::Level, string filename = "");
			protected:
				void renderAvailability(shared_ptr<Projector>, shared_ptr<World>, ofxRay::Camera & view, shared_ptr<Nodes::Base> scene);
				ofxCvGui::PanelGroupPtr panelGroup;

				Passes passes;

				struct : ofParameterGroup {
					ofParameter<float> brightness{ "Brightness", 1, 0, 10 };
					ofParameter<bool> autoRender{ "Auto render", true };
					PARAM_DECLARE("Projector", brightness, autoRender);
				} parameters;
			};
		}
	}
}
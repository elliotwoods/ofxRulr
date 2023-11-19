#pragma once

#include "../Nodes/Base.h"

#include "ofxCvGui/Panels/World.h"

#include "ofLight.h"

namespace ofxRulr {
	namespace Graph {
		class OFXRULR_API_ENTRY WorldStage : public Nodes::Base {
		public:
			WorldStage();
			string getTypeName() const override;
			void init();

			void update();
			ofxCvGui::PanelPtr getPanel() override;
			glm::vec3 getCursorWorld(bool forceUpdate = false) const;
			ofxGrabCam & getCamera();
			shared_ptr<ofxCvGui::Panels::WorldManaged> getPanelTyped();

		protected:
			void serialize(nlohmann::json &);
			void deserialize(const nlohmann::json &);
			void populateInspector(ofxCvGui::InspectArguments &);

			shared_ptr<ofxCvGui::Panels::WorldManaged> view;

			ofCamera * camera = nullptr;
			ofTexture * grid;
			ofLight light;
		};
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxOsc.h"
#include "ofxRulr/Data/Dosirak/Curve.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
			class Curves : public Nodes::Base {
			public:
				Curves();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				ofxCvGui::PanelPtr getPanel() override;
				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&) const;
				void deserialize(const nlohmann::json&);

				void setCurves(const Data::Dosirak::Curves&);
				const Data::Dosirak::Curves& getCurves() const;

				ofxLiquidEvent<Data::Dosirak::Curves> onNewCurves;

			protected:
				void updatePreview();

				struct Parameters : ofParameterGroup {
					ofParameter<float> scale{ "Scale", 1, 0.001, 100 };
					PARAM_DECLARE("Curves", scale);
				} parameters;

				shared_ptr<Item::RigidBody> rigidBody;

				ofxCvGui::PanelPtr panel;

				Data::Dosirak::Curves curves;
				struct {
					bool incoming = true;
					bool thisFrame = false;
				} newCurves;
				bool previewStale = true;
			};
		}
	}
}
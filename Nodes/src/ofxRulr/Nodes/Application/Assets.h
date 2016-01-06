#pragma once

#include "ofxRulr/Nodes/Base.h"

#include "ofxCvGui/Panels/Widgets.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			class Assets : public Base {
			public:
				Assets();
				string getTypeName() const override;
				
				void init();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(Json::Value &);
				void deserialize(const Json::Value &);
				
				ofxCvGui::PanelPtr getView() override;
			protected:
				void refreshView();
				
				shared_ptr<ofxCvGui::Panels::Widgets> view;
				vector<ofxCvGui::ElementPtr> ownedElements;
				ofParameter<string> filter;
			};
		}
	}
}
#pragma once

#include "NodeHost.h"
#include "LinkHost.h"

#include "ofxDigitalEmulsion/Graph/Node.h"
#include "ofxCvGui/Panels/ElementCanvas.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Patch {
			class Instance : public Graph::Node {
			public:
				class View : public ofxCvGui::Panels::ElementCanvas {
				public:
					View(Instance &);
					//const shared_ptr<ofxCvGui::Panels::Base> findScreen(const ofVec2f & xy, ofRectangle & currentPanelBounds) override;
				protected:
					void drawGridLines();
					Instance & patchInstance;
				};

				string getTypeName() const override;
				void init() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

				ofxCvGui::PanelPtr getView() override;
				void update() override;

				void addDebug();
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;

				map<NodeHost::Index, shared_ptr<NodeHost> > nodes;
				map<Link::Index, shared_ptr<Link> > links;
				shared_ptr<View> view;
			};
		}
	}
}
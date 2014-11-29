#pragma once

#include "NodeHost.h"
#include "LinkHost.h"

#include "ofxDigitalEmulsion/Graph/Node.h"
#include "ofxDigitalEmulsion/Graph/Factory.h"
#include "ofxCvGui/Panels/ElementCanvas.h"

namespace ofxDigitalEmulsion {
	namespace Graph {
		namespace Editor {
			class Patch : public Graph::Node {
			public:
				typedef map<NodeHost::Index, shared_ptr<NodeHost> > NodeHostSet;
				typedef map<LinkHost::Index, shared_ptr<LinkHost> > LinkHostSet;

				class View : public ofxCvGui::Panels::ElementCanvas {
				public:
					View(Patch &);
					//const shared_ptr<ofxCvGui::Panels::Base> findScreen(const ofVec2f & xy, ofRectangle & currentPanelBounds) override;
					void resync();
					shared_ptr<NodeHost> getNodeHostUnderCursor(const ofVec2f & cursorInCanvas);
				protected:
					ofVec2f lastCursorPositionInCanvas;
					void drawGridLines();
					Patch & patchInstance;
				};

				string getTypeName() const override;
				void init() override;

				void serialize(Json::Value &) override;
				void deserialize(const Json::Value &) override;

				ofxCvGui::PanelPtr getView() override;
				void update() override;
				void drawWorld() override;

				void rebuildLinkHosts();
				const NodeHostSet & getNodeHosts() const;
				const LinkHostSet & getLinkHosts() const;

				void addNode(NodeHost::Index index, shared_ptr<Node>);
				void addNewNode(shared_ptr<BaseFactory>);
				void addDebug();

				void deleteSelection();

				shared_ptr<TemporaryLinkHost> getNewLink() const;
				shared_ptr<NodeHost> findNodeHost(shared_ptr<Node>) const;
			protected:
				void populateInspector2(ofxCvGui::ElementGroupPtr) override;
				NodeHost::Index getNextFreeNodeHostIndex() const;
				LinkHost::Index getNextFreeLinkHostIndex() const;
				void callbackBeginMakeConnection(shared_ptr<NodeHost> targetNodeHost, shared_ptr<BasePin> targetPin);
				void callbackReleaseMakeConnection(ofxCvGui::MouseArguments &);

				NodeHostSet nodeHosts;
				LinkHostSet linkHosts;
				shared_ptr<View> view;

				shared_ptr<TemporaryLinkHost> newLink;
				weak_ptr<NodeHost> selection;
			};
		}
	}
}
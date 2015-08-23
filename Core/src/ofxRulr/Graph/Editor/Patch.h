#pragma once

#include "NodeHost.h"
#include "LinkHost.h"
#include "NodeBrowser.h"

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Graph/FactoryRegister.h"
#include "ofxCvGui/Panels/ElementCanvas.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			class Patch : public Nodes::Base, public enable_shared_from_this<Patch> {
			public:
				typedef map<NodeHost::Index, shared_ptr<NodeHost> > NodeHostSet;
				typedef map<LinkHost::Index, shared_ptr<LinkHost> > LinkHostSet;

				class View : public ofxCvGui::Panels::ElementCanvas {
				public:
					View(Patch &);
					//const shared_ptr<ofxCvGui::Panels::Base> findScreen(const ofVec2f & xy, ofRectangle & currentPanelBounds) override;
					void resync();

					shared_ptr<NodeHost> getNodeHostUnderCursor(const ofVec2f & cursorInCanvas);
					shared_ptr<NodeHost> getNodeHostUnderCursor();

					const ofxCvGui::PanelPtr findScreen(const ofVec2f & xy, ofRectangle & currentPanelBounds) override;
				protected:
					ofVec2f lastCursorPositionInCanvas;
					void drawGridLines();
					Patch & patchInstance;
					shared_ptr<NodeBrowser> nodeBrowser;
					ofVec2f birthLocation;
				};
				struct ExposedPin {
					weak_ptr<Graph::AbstractPin> pin;
					Nodes::Base * node;
				};
				typedef map<weak_ptr<AbstractPin>, ExposedPin, owner_less<std::weak_ptr<AbstractPin>>> ExposedPinSet;

				Patch();
				string getTypeName() const override;
				void init();

				void serialize(Json::Value &);
				void deserialize(const Json::Value &);

				void insertPatchlet(const Json::Value &, bool useNewIDs, ofVec2f offset = ofVec2f());

				ofxCvGui::PanelPtr getView() override;
				void update();
				void drawWorld() override;

				void rebuildLinkHosts();
				const NodeHostSet & getNodeHosts() const;
				const LinkHostSet & getLinkHosts() const;

				shared_ptr<NodeHost> addNode(NodeHost::Index index, shared_ptr<Nodes::Base>, const ofRectangle & bounds = ofRectangle());
				shared_ptr<NodeHost> addNode(shared_ptr<Nodes::Base>, const ofRectangle & bounds = ofRectangle());
				shared_ptr<NodeHost> addNewNode(shared_ptr<BaseFactory>, const ofRectangle & bounds = ofRectangle());

				void addNodeHost(shared_ptr<NodeHost>, int index);
				void addNodeHost(shared_ptr<NodeHost>);

				void deleteSelection();
				void cut();
				void copy();
				void paste();

				shared_ptr<TemporaryLinkHost> getNewLink() const;
				shared_ptr<NodeHost> findNodeHost(shared_ptr<Nodes::Base>) const;
				shared_ptr<NodeHost> getNodeHost(NodeHost::Index) const;

				void exposePin(shared_ptr<AbstractPin>, Nodes::Base *);
				void unexposePin(shared_ptr<AbstractPin>);
				const ExposedPinSet & getExposedPins() const;

				//delayed connection means perform the link on next Patch::update
				//this is useful when you are deserialising, so all subpatches deserialise before performing linking
				void connectPin(shared_ptr<AbstractPin>, NodeHost::Index nodeHostIndex, bool delayConnection);

				bool isRootPatch() const;
			protected:
				void populateInspector(ofxCvGui::ElementGroupPtr);

				NodeHost::Index getNextFreeNodeHostIndex() const;
				LinkHost::Index getNextFreeLinkHostIndex() const;
				void callbackBeginMakeConnection(shared_ptr<NodeHost> targetNodeHost, shared_ptr<AbstractPin> targetPin);
				void callbackReleaseMakeConnection(ofxCvGui::MouseArguments &);

				NodeHostSet nodeHosts;
				LinkHostSet linkHosts;
				shared_ptr<View> view;

				shared_ptr<TemporaryLinkHost> newLink;
				weak_ptr<NodeHost> selection;
				
				ExposedPinSet exposedPinSet;

				map<weak_ptr<AbstractPin>, NodeHost::Index, owner_less<weak_ptr<AbstractPin>>> delayedConnections;
			};
		}
	}
}
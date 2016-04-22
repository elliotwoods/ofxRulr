#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Data/Channels/Channel.h"

#include "ofxCvGui/Panels/Tree.h"
#include "ofxCvGui/Panels/Widgets.h"
#include "ofxCvGui/Panels/Groups/Strip.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			namespace Channels {
				namespace Generator {
					class Base;
				}

				using namespace ofxRulr::Data::Channels;

				class Database : public Nodes::Base {
				public:
					Database();
					string getTypeName() const override;

					void init();
					void update();
					void populateInspector(ofxCvGui::InspectArguments &);

					ofxCvGui::PanelPtr getPanel();

					shared_ptr<Channel> getRootChannel();
					void clear();

					void addGenerator(shared_ptr<Nodes::Data::Channels::Generator::Base>);
					void removeGenerator(Nodes::Data::Channels::Generator::Base *);

					ofxLiquidEvent<Channel> onPopulateData;
				protected:
					void rebuildTree();
					void rebuildDetailView();
					void addGenerationToGui(shared_ptr<ofxCvGui::Panels::Tree::Branch>, const Channel &);
					void selectChannel(weak_ptr<Channel>);

					shared_ptr<Channel> rootChannel;

					shared_ptr<ofxCvGui::Panels::Groups::Strip> view;
					shared_ptr<ofxCvGui::Panels::Tree> treeView;
					shared_ptr<ofxCvGui::Panels::Widgets> detailView;

					bool needsRebuild = true;

					vector<weak_ptr<Nodes::Data::Channels::Generator::Base>> generators;

					weak_ptr<Channel> selectedChannel;
				};
			}
		}
	}
}
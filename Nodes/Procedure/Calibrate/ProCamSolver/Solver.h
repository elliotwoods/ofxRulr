#pragma once
#include "ofxRulr/Nodes/Base.h"

#include "Node.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					class AddView;
					class AddScan;

					class Solver : public Base {
					public:
						Solver();
						string getTypeName() const override;
						ofxCvGui::PanelPtr getPanel() override;
						
						void init();
						void update();
						void populateInspector(ofxCvGui::InspectArguments &);

						void addNode(shared_ptr<Node>);
						void removeNode(shared_ptr<Node>);

						void solve();

						vector<shared_ptr<AddView>> getViews() const;
						vector<shared_ptr<AddScan>> getScans() const;
					protected:
						void rebuildPanel();

						shared_ptr<ofxCvGui::Panels::Widgets> panel;
						vector<weak_ptr<Node>> nodes;

						bool panelDirty = true;
					};
				}
			}
		}
	}
}
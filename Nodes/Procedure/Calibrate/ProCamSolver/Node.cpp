#include "pch_RulrNodes.h"
#include "Node.h"
#include "Solver.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					//----------
					Node::Node() {
						RULR_NODE_INIT_LISTENER;
					}

					//----------
					string Node::getTypeName() const {
						return "Procedure::Calibrate::ProCamSolver::Node";
					}

					//----------
					void Node::init() {
						{
							auto solverPin = this->addInput<Solver>();
							solverPin->onNewConnection += [this](shared_ptr<Solver> solver) {
								solver->addNode(this->shared_from_this());
							};
							solverPin->onDeleteConnection += [this](shared_ptr<Solver> solver) {
								if (solver) {
									solver->removeNode(this->shared_from_this());
								}
							};
						}

						{
							auto widget = make_shared<ofxCvGui::Element>();
							widget->onDraw += [this](ofxCvGui::DrawArguments &) {
								auto node = this->getNode();
								string label;
								if (!node) {
									label = "Not connected";
								}
								else {
									label = node->getName();
								}
								ofxCvGui::Utils::drawText(label, 0, 0, false, 30);
							};
							widget->setBounds(ofRectangle(0, 0, 100, 30));
							this->widget = widget;
						}
					}

					//----------
					ofxCvGui::ElementPtr Node::getWidget() {
						return this->widget;
					}
				}
			}
		}
	}
}
#include "pch_RulrNodes.h"
#include "Solver.h"

#include "AddView.h"
#include "AddScan.h"
#include "ProCamModel.h"

#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"


namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					//----------
					Solver::Solver() {
						RULR_NODE_INIT_LISTENER;
					}

					//----------
					string Solver::getTypeName() const {
						return "Calibrate::ProCamSolver::Solver";
					}

					//----------
					ofxCvGui::PanelPtr Solver::getPanel() {
						return this->panel;
					}

					//----------
					void Solver::init() {
						RULR_NODE_UPDATE_LISTENER;
						RULR_NODE_INSPECTOR_LISTENER;

						this->panel = make_shared<ofxCvGui::Panels::Widgets>();
					}

					//----------
					void Solver::update() {
						if (this->panelDirty) {
							this->rebuildPanel();
						}
					}

					//----------
					void Solver::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
						auto inspector = inspectArgs.inspector;

						{
							auto solveButton = inspector->addButton("Solve", [this]() {
								try {
									this->solve();
								}
								RULR_CATCH_ALL_TO_ALERT;
							});
							solveButton->setHeight(100.0f);
							solveButton->setHotKey(OF_KEY_RETURN);
						}
					}

					//----------
					void Solver::addNode(shared_ptr<Node> node) {
						auto weakNode = weak_ptr<Node>(node);
						this->nodes.push_back(weakNode);
						this->panelDirty = true;
					}

					//----------
					void Solver::removeNode(shared_ptr<Node> node) {
						for (auto it = this->nodes.begin(); it != this->nodes.end(); ) {
							auto otherNode = it->lock();
							if (otherNode == node) {
								it = this->nodes.erase(it);
							}
							else {
								it++;
							}
						}
						this->panelDirty = true;
					}
					
					//----------
					void Solver::rebuildPanel() {
						this->panel->clear();

						auto views = this->getViews();
						auto scans = this->getScans();
						this->panel->addTitle("Views [" + ofToString(views.size()) + "]");
						for (auto view : views) {
							this->panel->add(view->getWidget());
						}

						this->panel->addTitle("Scans [" + ofToString(scans.size()) + "]"); 
						for (auto scan : scans) {
							this->panel->add(scan->getWidget());
						}

						this->panelDirty = false;
					}

					//----------
					void Solver::solve() {
						Utils::ScopedProcess scopedProcess("ProCam Solve", false);

						auto views = this->getViews(); 
						auto scans = this->getScans();

						auto fitPoints = scans[0]->getFitPoints();
						ProCamSolver::Model model(views[0], views[1]);

						{
							Utils::ScopedProcess fitProcess("Fit views to rays");
							//ofxNonLinearFit::Fit<Model> fit(model.getParameterCount(), ofxNonLinearFit::Algorithm(nlopt::algorithm::LN_NEWUOA_BOUND));

							ofxNonLinearFit::Fit<Model> fit(model.getParameterCount(), ofxNonLinearFit::Algorithm(nlopt::algorithm::GN_MLSL));
							ofxNonLinearFit::Fit<Model> localFit(model.getParameterCount(), ofxNonLinearFit::Algorithm(nlopt::algorithm::LN_NEWUOA));
							nlopt_set_local_optimizer(fit.getOptimiser(), localFit.getOptimiser());
							nlopt_set_maxtime(fit.getOptimiser(), 60 * 10); // 10mins max

							//set bounds
							{
								auto & optimizer = fit.getOptimiser();
								auto lowerBounds = model.getLowerBounds();
								auto upperBounds = model.getUpperBounds();
								nlopt_set_lower_bounds(optimizer, lowerBounds.data());
								nlopt_set_upper_bounds(optimizer, upperBounds.data());
							}

							double residual = 0;
							if (fit.optimise(model, &fitPoints, &residual)) {
								fitProcess.end();
							}

							cout << "residual : " << residual << endl;
						}
					}

					//----------
					vector<shared_ptr<AddView>> Solver::getViews() const {
						vector<shared_ptr<AddView>> views;

						for (auto nodeWeak : this->nodes) {
							auto node = nodeWeak.lock();
							auto view = dynamic_pointer_cast<AddView>(node);
							if (view) {
								views.push_back(view);
							}
						}
						return views;
					}

					//----------
					vector<shared_ptr<AddScan>> Solver::getScans() const {
						vector<shared_ptr<AddScan>> scans;

						for (auto nodeWeak : this->nodes) {
							auto node = nodeWeak.lock();
							auto scan = dynamic_pointer_cast<AddScan>(node);
							if (scan) {
								scans.push_back(scan);
							}
						}
						return scans;
					}

				}
			}
		}
	}
}
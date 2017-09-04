#include "pch_RulrNodes.h"
#include "AddView.h"
#include "Solver.h"

#include "ofxRulr/Nodes/Item/View.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace ProCamSolver {
					//----------
					AddView::AddView() {
						RULR_NODE_INIT_LISTENER;
					}

					//----------
					string AddView::getTypeName() const {
						return "Calibrate::ProCamSolver::AddView";
					}

					//----------
					void AddView::init() {
						RULR_NODE_SERIALIZATION_LISTENERS;
						RULR_NODE_INSPECTOR_LISTENER;

						this->addInput<Item::View>();

						this->position[0] = FitParameter{ "Position x", true, 10.0f };
						this->position[1] = FitParameter{ "Position y", true, 10.0f };
						this->position[2] = FitParameter{ "Position z", true, 10.0f };

						this->rotation[0] = FitParameter{ "Rotation x", true, 90.0f };
						this->rotation[1] = FitParameter{ "Rotation y", true, 90.0f };
						this->rotation[2] = FitParameter{ "Rotation z", true, 90.0f };

						this->throwRatioLog = FitParameter{ "log(ThrowRatio)", true, 5.0f };
						this->pixelAspectRatioLog = FitParameter{ "log(PixelAspectRatio)", false, 1.0f };

						this->lensOffset[0] = FitParameter{ "Lens Offset X", false, 0.5f };
						this->lensOffset[1] = FitParameter{ "Lens Offset Y", true, 0.5f };

						for (int i = 0; i < RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
							this->distortion[i] = FitParameter{ "Distortion " + ofToString(i), false, 2.0f };
						}

						this->fitParameters.insert(this->fitParameters.end(), {
							&this->position[0],
							&this->position[1],
							&this->position[2],

							&this->rotation[0],
							&this->rotation[1],
							&this->rotation[2],

							&this->throwRatioLog,
							&this->pixelAspectRatioLog,

							&this->lensOffset[0],
							&this->lensOffset[1],

							&this->distortion[0],
							&this->distortion[1],
							&this->distortion[2],
							&this->distortion[3]
						});
					}

					//----------
					void AddView::serialize(Json::Value & json) {
						auto & fitParametersJson = json["fitParameters"];
						for (auto fitParameter : this->fitParameters) {
							auto & fitParameterJson = fitParametersJson[fitParameter->name];
							fitParameterJson["enabled"] << fitParameter->enabled;
							fitParameterJson["deviation"] << fitParameter->deviation;
						}
					}

					//----------
					void AddView::deserialize(const Json::Value & json) {
						const auto & fitParametersJson = json["fitParameters"];
						for (auto fitParameter : this->fitParameters) {
							auto & fitParameterJson = fitParametersJson[fitParameter->name];
							if (fitParameterJson["enabled"].isString()) {
								fitParameterJson["enabled"] >> fitParameter->enabled;
							}
							if (fitParameterJson["deviation"].isString()) {
								fitParameterJson["deviation"] >> fitParameter->deviation;
							}
						}
					}

					//----------
					void AddView::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
						auto inspector = inspectArgs.inspector;
						{
							inspector->addTitle("Fit Parameters", ofxCvGui::Widgets::Title::H2);

							{
								auto chooseFixed = inspector->addMultipleChoice("Fixed");
								chooseFixed->addOption("All fixed");
								chooseFixed->addOption("All unfixed");
								chooseFixed->addOption("Some fixed");

								auto chooseFixedWeak = weak_ptr<ofxCvGui::Widgets::MultipleChoice>(chooseFixed);
								chooseFixed->onUpdate += [this, chooseFixedWeak](ofxCvGui::UpdateArguments &) {
									auto chooseFixed = chooseFixedWeak.lock();
									if (chooseFixed) {
										bool anyUnfixed = false;
										bool anyFixed = false;
										for (auto fitParameter : this->fitParameters) {
											if (fitParameter->enabled) {
												anyUnfixed = true;
											}
											else {
												anyFixed = true;
											}
										}
										if (anyUnfixed && anyFixed) {
											chooseFixed->setSelection(2);
										}
										else if (anyUnfixed) {
											chooseFixed->setSelection(1);
										}
										else if (anyFixed) {
											chooseFixed->setSelection(0);
										}
									}
								};

								chooseFixed->onValueChange += [this](const int & selection) {
									switch (selection) {
									case 0:
										for (auto fitParameter : this->fitParameters) {
											fitParameter->enabled = false;
										}
										break;
									case 1:
										for (auto fitParameter : this->fitParameters) {
											fitParameter->enabled = true;
										}
										break;
									default:
										break;
									}
								};
							}
							

							for (auto fitParameter : this->fitParameters) {
								inspector->addTitle(fitParameter->name, ofxCvGui::Widgets::Title::H3);
								inspector->addToggle("Enabled"
									, [fitParameter]() {
									return fitParameter->enabled;
								}
									, [fitParameter](bool value) {
									fitParameter->enabled = value;
								});
								inspector->addEditableValue<float>("Deviation"
									, [fitParameter]() {
									return fitParameter->deviation;
								}
									, [fitParameter](string valueString) {
									if (!valueString.empty()) {
										fitParameter->deviation = ofToFloat(valueString);
									}
								});
							}
						}
					}

					//----------
					vector<FitParameter *> AddView::getActiveFitParameters() {
						auto view = this->getInput<Item::View>();

						for (int i = 0; i < 3; i++) {
							this->position[i].get = [view, i]() {
								return view->getPosition()[i];
							};
							this->position[i].set = [view, i](float value) {
								auto position = view->getPosition();
								position[i] = value;
								view->setPosition(position);
							};
						}

						for (int i = 0; i < 3; i++) {
							this->rotation[i].get = [view, i]() {
								return view->getRotationEuler()[i];
							};
							this->rotation[i].set = [view, i](float value) {
								auto rotation = view->getRotationEuler();
								rotation[i] = value;
								view->setRotationEuler(rotation);
							};
						}

						this->throwRatioLog.get = [view]() {
							return log(view->getThrowRatio());
						};
						this->throwRatioLog.set = [view](float value) {
							view->setThrowRatio(exp(value));
						};

						this->pixelAspectRatioLog.get = [view]() {
							return log(view->getPixelAspectRatio());
						};
						this->pixelAspectRatioLog.set = [view](float value) {
							view->setPixelAspectRatio(exp(value));
						};

						for (int i = 0; i < 2; i++) {
							this->lensOffset[i].get = [view, i]() {
								return view->getLensOffset()[i];
							};
							this->lensOffset[i].set = [view, i](float value) {
								auto lensOffset = view->getLensOffset();
								lensOffset[i] = value;
								view->setLensOffset(lensOffset);
							};
						}


						for (int i = 0; i < RULR_VIEW_DISTORTION_COEFFICIENT_COUNT; i++) {
							if (this->distortion[i].enabled) {
								throw(ofxRulr::Exception("ProCamSolver currently doesn't support distortion."));
							}
						}

						vector<FitParameter *> activeFitParameters;
						for (auto fitParameter : this->fitParameters) {
							if (fitParameter->enabled) {
								activeFitParameters.push_back(fitParameter);
							}
						}
						return activeFitParameters;
					}

					//----------
					shared_ptr<ofxRulr::Nodes::Base> AddView::getNode() {
						return this->getInput<Item::View>();
					}
				}
			}
		}
	}
}
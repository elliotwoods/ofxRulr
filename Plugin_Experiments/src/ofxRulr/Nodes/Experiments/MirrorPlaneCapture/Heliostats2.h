#pragma once

#include "pch_Plugin_Experiments.h"
#include "ofxRulr/Solvers/HeliostatActionModel.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class Heliostats2 : public Nodes::Base {
				public:
					struct HAMParameters : ofParameterGroup {
						struct AxisParameters : ofParameterGroup {
							// Rotation away from -y axis
							ofParameter<glm::vec3> rotationAxis{ "Rotation axis", {0.0f, -1.0f, 0.0f} };

							// X, X*X, X*X*X
							ofParameter<glm::vec3> polynomial{ "Polynomial", {0, 1, 0} };

							PARAM_DECLARE("AxisParameters", rotationAxis, polynomial);
						};

						// Pivot point
						ofParameter<glm::vec3> position{ "Position", {0, 0, 0} };

						// Rotation
						ofParameter<float> rotationY{ "Rotation Y", 0, -180, 180 };

						AxisParameters axis1;
						AxisParameters axis2;
						ofParameter<float> mirrorOffset{ "Mirror offset", 0.136, 0.1, 0.2 };

						PARAM_DECLARE("HAM", position, rotationY, mirrorOffset);

						HAMParameters() {
							// the axis points out of the motor face
							axis1.setName("Axis 1");
							axis2.setName("Axis 2");
							axis2.rotationAxis.set({ 1.0f, 0, 0 });
							this->add(axis1);
							this->add(axis2);
						}
					};

					struct ServoParameters : ofParameterGroup {
						ofParameter<int> ID{ "ID", 1 };
						ofParameter<float> angle{ "Angle", 0, -180, 180 };
						ofParameter<int> goalPosition{ "GoalPosition", 4096 / 2 };

						ServoParameters(const string& name, HAMParameters::AxisParameters&);
						
						void update();
						bool getGoalPositionNeedsPush() const;
						void markGoalPositionPushed();

						void calculateGoalPosition();
						void setPresentPosition(const Dispatcher::RegisterValue&);
						void setMinPosition(const Dispatcher::RegisterValue&);
						void setMaxPosition(const Dispatcher::RegisterValue&);

					protected:
						HAMParameters::AxisParameters& axisParameters;
						float cachedAngle = 0.0f;
						int cachedGoalPosition = 0;

						bool goalPositionNeedsPush = true;
					};

					class Heliostat : public Utils::AbstractCaptureSet::BaseCapture, public ofxCvGui::IInspectable, public enable_shared_from_this<Heliostat> {
					public:
						struct DrawParameters {
							bool nodeIsSelected;
							bool labels;
							bool servoIndices;
						};

						Heliostat();
						string getDisplayString() const override;
						void drawWorld(const DrawParameters&);
						void drawMirrorFace();

						struct Parameters : ofParameterGroup {
							ofParameter<string> name{ "Name", "" };

							ServoParameters servo1;
							ServoParameters servo2;

							HAMParameters hamParameters;

							ofParameter<float> diameter{ "Diameter", 0.35f, 0.0f, 1.0f };

							Parameters()
							: servo1("Servo 1", this->hamParameters.axis1)
							, servo2("Servo 2", this->hamParameters.axis2) {
							}

							PARAM_DECLARE("Heliostat"
								, name
								, servo1
								, servo2
								, hamParameters
								, diameter);
						} parameters;
						
						string getName() const override;
						void update();
						void serialize(nlohmann::json&);
						void deserialize(const nlohmann::json&);
						void populateInspector(ofxCvGui::InspectArguments&);
						Solvers::HeliostatActionModel::Parameters<float> getHeliostatActionModelParameters() const;
						void setHeliostatActionModelParameters(const Solvers::HeliostatActionModel::Parameters<float> &);

						void flip();

						void navigateToNormal(const glm::vec3&, const ofxCeres::SolverSettings&, bool throwIfOutsideRange);
						void navigateToReflectPointToPoint(const glm::vec3&, const glm::vec3&, const ofxCeres::SolverSettings&, bool throwIfOutsideRange);
					protected:
						ofxCvGui::ElementPtr getDataDisplay() override;
					};

					Heliostats2();
					~Heliostats2();
					string getTypeName() const override;

					void init();
					void update();

					void populateInspector(ofxCvGui::InspectArguments&);
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					ofxCvGui::PanelPtr getPanel() override;
					void drawWorldStage();

					vector<shared_ptr<Heliostat>> getHeliostats();

					void add(shared_ptr<Heliostat>);
					void removeHeliostat(shared_ptr<Heliostat> heliostat);
					void importCSV();

					void faceAllTowardsCursor();
					void faceAllAway();

					void pushStale(bool blocking, bool waitUntilComplete);
					void pushAll(bool blocking);
					void pullAllPositions();
					void pullAllLimits();

					void resetDetailParameters();

					void selectRangeByString(const string&);

					cv::Mat drawMirrorFaceMask(shared_ptr<Heliostat>, const ofxRay::Camera&);

				protected:
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> printReport{ "Print report", true };
							PARAM_DECLARE("Navigator", printReport);
						} navigator;

						struct : ofParameterGroup {
							ofParameter<bool> pushStaleValues{ "Push stale every frame", true };
							ofParameter<float> timeout{ "Timeout", 10.0f };
							PARAM_DECLARE("Dispatcher", pushStaleValues, timeout);
						} dispatcher;

						struct : ofParameterGroup {
							ofParameter<float> servo1{ "Servo 1", 0, -180, 180 };
							ofParameter<float> servo2{ "Servo 2", 0, -100, 100 };
							PARAM_DECLARE("Away pose", servo1, servo2);
						} awayPose;

						ofParameter<WhenDrawOnWorldStage> trackCursor{ "Track cursor", WhenDrawOnWorldStage::Never };

						struct : ofParameterGroup {
							ofParameter<WhenDrawOnWorldStage> labels{ "Labels", WhenDrawOnWorldStage::Selected };
							ofParameter<bool> servoIndices{ "Servo indices", false };
							PARAM_DECLARE("Draw", labels, servoIndices);
						} draw;

						PARAM_DECLARE("Heliostats2", navigator, dispatcher, awayPose, draw, trackCursor);
					} parameters;

					Utils::CaptureSet<Heliostat> heliostats;
					shared_ptr<ofxCvGui::Panels::Widgets> panel;

					struct DispatcherThread {
						struct Action {
							Action(const function<void()>& action, bool isPushStale)
								: action(action)
								, containsPushStale(containsPushStale)
								, future(promise.get_future())
							{

							}

							function<void()> action;
							bool containsPushStale;
							promise<void> promise;
							future<void> future;
						};
						std::deque<shared_ptr<Action>> actions;
						std::mutex actionsLock;
						condition_variable actionsCV;
						std::thread thread;
						bool isRunning = false;
						bool isClosing = false;
					} dispatcherThread;
				};
			}
		}
	}
}
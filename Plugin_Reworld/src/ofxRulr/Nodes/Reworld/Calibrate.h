#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Data/Reworld/Capture.h"

#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Data/Reworld/Module.h"
#include "ofxRulr/Solvers/Reworld/Navigate/PointToPoint.h"

#include "Router.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class CalibrateController;
			class Installation;

			class Calibrate : public Base
			{
			public:
				struct CalibrateControllerSession {
				public:
					CalibrateControllerSession();
					Data::Reworld::ColumnIndex getColumnIndex() const;
					Data::Reworld::ModuleIndex getModuleIndex() const;
					void setColumnIndex(Data::Reworld::ColumnIndex);
					void setModuleIndex(Data::Reworld::ModuleIndex);
					ofColor getColor() const;
					
					void setName(const string&);
					string getName() const;

					ofxLiquidEvent<void> onSelectedModuleChange;
				protected:
					Data::Reworld::ColumnIndex columnIndex = 0;
					Data::Reworld::ModuleIndex moduleIndex = 0;
					ofColor color;
					string name = "Controller";
				};

				Calibrate();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				ofxCvGui::PanelPtr getPanel() override;

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments);

				shared_ptr<CalibrateControllerSession> registerCalibrateController(CalibrateController *);
				void unregisterCalibrateController(CalibrateController*);
				vector<shared_ptr<CalibrateControllerSession>> getCalibrateControllerSessions() const;
				map<Router::Address, vector<string>> getCalibrateControllerSessionsNameCache() const;

				const set<Router::Address> & getSelectedModuleIndicesCache() const;

				void initCaptures();
				void initCapture(shared_ptr<Data::Reworld::Capture>);
				shared_ptr<Data::Reworld::Capture> newCapture();

				void estimateModuleData(Data::Reworld::Capture*);
				void moveToCapturePositions(Data::Reworld::Capture*);
				void moveDataPoint(shared_ptr<CalibrateControllerSession>, const glm::vec2&);
				void markDataPointGood(shared_ptr<CalibrateControllerSession>);
				void clearModuleDataSetValues(shared_ptr<CalibrateControllerSession>);
				Utils::EditSelection<Data::Reworld::Capture> ourSelection;

				void calculateParking();

				struct GatheredData {
					vector<shared_ptr<Data::Reworld::Module>> modules;
					vector<pair<Data::Reworld::ColumnIndex, Data::Reworld::ModuleIndex>> flatToIndexed;
					vector<shared_ptr<Data::Reworld::Capture>> captures;
					vector<glm::vec3> targetPositions;
				};

				void gatherData(GatheredData& data) const;
				void calibrate();
				void calculateResiduals();

				void clearPerModuleCalibrations();

			protected:
				struct : ofParameterGroup {
					

					struct : ofParameterGroup {
						ofParameter<bool> controlOn{ "Control ON", true };

						struct : ofParameterGroup {
							ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::Reworld::Navigate::PointToPoint::defaultSolverSettings() };
							
							struct : ofParameterGroup {
								ofParameter<bool> enabled{ "Enabled", true };
								ofParameter<float> deadZone{ "Dead zone", 0.05, 0, 1 };
								ofParameter<float> maxResidual{ "Max residual", 0.1, 0, 1 };
								PARAM_DECLARE("Filter modules", enabled, deadZone, maxResidual);
							} filterModules;

							PARAM_DECLARE("Estimation", solverSettings, filterModules);
						} estimation;

						// Parking is for moving other spots out of the way whilst calibrating and for showing status
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							struct : ofParameterGroup {
								ofParameter<float> unselected{ "Unselected", 0.5, -1, 1 };
								ofParameter<float> estimated{ "Estimated", -0.2, -1, 1 };
								ofParameter<float> set{ "Set", -0.1, -1, 1 };
								ofParameter<float> good{ "Good", 0.1, -1, 1 };
								PARAM_DECLARE("Parking spots", unselected, estimated, set, good);
							} parkingSpots;
							PARAM_DECLARE("Park non-selected", enabled, parkingSpots);
						} parkNonSelected;

						PARAM_DECLARE("Control", controlOn, estimation, parkNonSelected)
					} control;

					struct : ofParameterGroup {
						ofParameter<bool> fixLightPosition{ "Fix light position", false };
						ofParameter<bool> fixInterPrismDistance{ "Fix inter-prism distance", false };
						ofParameter<bool> fixPrismAngle{ "Fix prism angle", false };
						ofParameter<bool> fixIOR{ "Fix IOR", false };

						ofParameter<bool> fixAllModulePositions{ "Fix all module positions", false };
						ofParameter<bool> fixAllModuleRotations{ "Fix all module rotations", false };
						ofParameter<bool> fixAllAxisAngleOffsets{ "Fix all axis-angle offsets", false };

						ofxCeres::ParameterisedSolverSettings solverSettings;
						ofParameter<int> minDataPointsPerModule{ "Minimum datapoints per module", 1 };

						ofParameter<float> solveResidual{ "Solve residual", 0.0f };

						PARAM_DECLARE("solve", fixLightPosition
							, fixInterPrismDistance
							, fixPrismAngle
							, fixIOR
							, fixAllModulePositions
							, fixAllModuleRotations
							, fixAllAxisAngleOffsets
							, solverSettings
							, minDataPointsPerModule
							, solveResidual);
					} solve;

					PARAM_DECLARE("Calibrate", control, solve);
				} parameters;

				set<Router::Address> selectedModuleIndicesCache;

				map<CalibrateController*, shared_ptr<CalibrateControllerSession>> calibrateControllerSessions;
				map<Router::Address, vector<string>> calibrateControllerSessionsNameCache;

				Utils::CaptureSet<Data::Reworld::Capture> captures;

				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				bool needsLoadCaptureData = false;
				bool calibrateControllerSessionDataChanged = true;
				bool needsCalculateParking = true;

				vector<string> oscOutbox;

				float lastResidual = 0.0f;
			};
		}
	}
}
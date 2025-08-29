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
					Data::Reworld::ColumnIndex columnIndex = 0;
					Data::Reworld::ModuleIndex moduleIndex = 0;
					ofColor debugColor{ 255, 0, 0 };
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
				const set<Router::Address> & getCalibrateControllerSelections() const;

				void initCaptures();
				void initCapture(shared_ptr<Data::Reworld::Capture>);
				shared_ptr<Data::Reworld::Capture> newCapture();

				void estimateModuleData(Data::Reworld::Capture*);
				void moveToCapturePositions(Data::Reworld::Capture*);
				void moveDataPoint(shared_ptr<CalibrateControllerSession>, const glm::vec2&);
				void markDataPointGood(shared_ptr<CalibrateControllerSession>);
				Utils::EditSelection<Data::Reworld::Capture> ourSelection;
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofxCeres::ParameterisedSolverSettings solverSettings{ Solvers::Reworld::Navigate::PointToPoint::defaultSolverSettings() };
						PARAM_DECLARE("Initialisation", solverSettings);
					} initialisation;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<float> axisOffset{ "Axis offset", 0.1, -1, 1 };
							PARAM_DECLARE("Park non-selected", enabled);
						} parkNonSelected;
						PARAM_DECLARE("Control", parkNonSelected)
					} control;

					PARAM_DECLARE("Calibrate", initialisation, control);
				} parameters;

				map<CalibrateController*, shared_ptr<CalibrateControllerSession>> calibrateControllerSessions;
				set<Router::Address> calibrateControllerSelections;

				Utils::CaptureSet<Data::Reworld::Capture> captures;

				shared_ptr<ofxCvGui::Panels::Widgets> panel;

				bool needsLoadCaptureData = false;
			};
		}
	}
}
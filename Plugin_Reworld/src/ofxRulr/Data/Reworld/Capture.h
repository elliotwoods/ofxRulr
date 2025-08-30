#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Models/Reworld/Module.h"

#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Data/Reworld/Module.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Calibrate;
		}
	}

	namespace Data {
		namespace Reworld {
			struct Capture : public Utils::AbstractCaptureSet::BaseCapture {
				struct ModuleDataPoint {
					enum State : int {
						Unset = 0
						, Estimated = 1
						, Set = 2
						, Good = 3
					};

					State state = State::Unset;
					Models::Reworld::AxisAngles<float> axisAngles;

					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);
				};

				Capture();
				string getTypeName() const override;
				string getDisplayString() const override;

				void setParent(Nodes::Reworld::Calibrate*);
				void init();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments);
				void drawWorldStage();

				// Called if we're viewing this capture
				void updatePanel();

				void setTarget(const glm::vec3&);
				const glm::vec3& getTarget() const;
				bool getTargetIsSet() const;

				void initialiseModuleDataWithEstimate(ColumnIndex, ModuleIndex, const Module::AxisAngles&);
				void setManualModuleData(ColumnIndex, ModuleIndex, const Module::AxisAngles&);
				void markDataPointGood(ColumnIndex, ModuleIndex);
				void clearAllModuleData();

				shared_ptr<ModuleDataPoint> getModuleDataPoint(ColumnIndex, ModuleIndex);

				ofxLiquidEvent<void> onModuleDataPointsChange;

				void populatePanel(shared_ptr<ofxCvGui::Panels::Groups::Strip>);

				string getText() const;

				map<ColumnIndex, map<ModuleIndex, shared_ptr<ModuleDataPoint>>> moduleDataPoints;
				ofParameter<glm::vec3> target{ "Target", {0, 0, 0} };
				ofParameter<string> comment{ "Comment", "" };
				bool targetIsSet = false;

				Utils::EditSelection<Capture>* parentSelection = nullptr;
				Nodes::Reworld::Calibrate* parent = nullptr;

				shared_ptr<ofxCvGui::Panels::Groups::Strip> columnsPanel;
				bool columnsPanelDirty = true;
			protected:
				void rebuildColumnsPanel();
				ofxCvGui::ElementPtr getDataDisplay() override;

			};
		}
	}
}
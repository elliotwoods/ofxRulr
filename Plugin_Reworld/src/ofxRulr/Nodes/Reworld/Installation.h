#pragma once

#include "ofxRulr/Nodes/IHasVertices.h"
#include "ofxRulr/Data/Reworld/Column.h"
#include "ofxRulr/Utils/EditSelection.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class Installation : public Item::RigidBody
			{
			public:
				struct LabelDraws : ofParameterGroup {
					ofParameter<WhenActive> column{ "Column", WhenActive::Always };
					ofParameter<WhenActive> module{ "Module", WhenActive::Selected };
					PARAM_DECLARE("Labels", column, module);
				};

				struct PhysicalParameters : ofParameterGroup {
					ofParameter<float> interPrismDistanceMM{ "Inter prism distance [mm]", 18.8, 0, 100};
					ofParameter<float> prismAngle{ "Prism angle degrees", 30, 0, 90 };
					ofParameter<float> ior{ "IOR", 1.491, 1, 3 }; // 1.491 = acrylic
					PARAM_DECLARE("Physical parameters", interPrismDistanceMM, prismAngle, ior);
				};

				Installation();
				string getTypeName() const override;

				void init();
				void update();
				void drawObjectAdvanced(DrawWorldAdvancedArgs&);

				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);
				ofxCvGui::PanelPtr getPanel() override;

				const PhysicalParameters& getPhysicalParameters() const;
				void setPhysicalParameters(const PhysicalParameters&);

				void build();
				void initColumns();

				void selectAllModules();

				void pushAllValues(bool forceSend, bool selectedOnly);

				void exportPositionsCSV() const;

				Utils::EditSelection<Data::Reworld::Column> ourSelection;

				vector<shared_ptr<Data::Reworld::Module>> getSelectedModules() const;

				vector<shared_ptr<Data::Reworld::Column>> getAllColumns() const;
				shared_ptr<Data::Reworld::Column> getColumnByIndex(Data::Reworld::ColumnIndex, bool selectedOnly) const;
				shared_ptr<Data::Reworld::Module> getModuleByIndices(Data::Reworld::ColumnIndex, Data::Reworld::ModuleIndex, bool selectedOnly) const;
				map<Data::Reworld::ColumnIndex, shared_ptr<Data::Reworld::Column>> getSelectedColumnByIndex() const;
				map<Data::Reworld::ColumnIndex, map<Data::Reworld::ModuleIndex, shared_ptr<Data::Reworld::Module>>> getSelectedModulesByIndex() const;
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<int> countX{ "Count X", 4*2*3 };
							ofParameter<int> countY{ "Count Y", 6*3 };
							ofParameter<float> pitch{ "Pitch", 0.14, 0, 1 };
							PARAM_DECLARE("Basic", countX, countY, pitch);
						} basic;

						// Special step every section (e.g. every panel/window/etc)
						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<size_t> sectionSize{ "Section size", 8 };
							PARAM_DECLARE("Section step", enabled, sectionSize);
						} sectionStep;

						PARAM_DECLARE("Builder", basic, sectionStep);
					} builder;

					PhysicalParameters physicalParameters;

					struct : ofParameterGroup {
						ofParameter<bool> onChange{ "On change", true };
						ofParameter<float> onPeriod{ "On period [s]", 1, 0, 120 };
						ofParameter<bool> periodEnabled{ "Period enabled", true };
						PARAM_DECLARE("Transmit", onChange, onPeriod, periodEnabled);
					} transmit;

					struct : ofParameterGroup {
						LabelDraws labels;
						PARAM_DECLARE("Draw", labels);
					} draw;

					PARAM_DECLARE("Installation", builder, physicalParameters, transmit, draw);
				} parameters;

				shared_ptr<ofxCvGui::Panels::Widgets> panel;
				shared_ptr<Nodes::Item::RigidBody> stepOffset;

				Utils::CaptureSet<Data::Reworld::Column> columns;

				struct {
					float lastSendTime = 0.0f;
				} transmit;
			};
		}
	}
}
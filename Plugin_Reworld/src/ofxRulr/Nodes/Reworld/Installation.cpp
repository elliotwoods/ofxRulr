#include "pch_Plugin_Reworld.h"
#include "Installation.h"
#include "ColumnView.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//---------
			Installation::Installation()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string
				Installation::getTypeName() const
			{
				return "Reworld::Installation";
			}

			//---------
			void
				Installation::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;


				this->onDrawObjectAdvanced += [this](DrawWorldAdvancedArgs& args) {
					this->drawObjectAdvanced(args);
					};

				this->stepOffset = make_shared<Nodes::Item::RigidBody>();
				this->stepOffset->init();


				this->manageParameters(this->parameters);

				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					panel->addTitle("Columns:", ofxCvGui::Widgets::Title::Level::H3);
					this->columns.populateWidgets(panel);
					this->panel = panel;
				}
			}

			//---------
			void
				Installation::update()
			{

			}

			//---------
			void
				Installation::drawObjectAdvanced(DrawWorldAdvancedArgs& args)
			{
				auto argsCustom = args;
				argsCustom.custom = &this->parameters.draw.labels;
				auto columns = this->columns.getSelection();

				for (auto column : columns) {
					// Enable and disable the caption for the RigidBody
					argsCustom.enableGui = ofxRulr::isActive(Nodes::Reworld::ColumnView::isSelected(column.get()), this->parameters.draw.labels.column.get());
					column->drawWorldAdvanced(argsCustom);
				}
			}

			//---------
			void
				Installation::serialize(nlohmann::json& json)
			{
				this->columns.serialize(json["columns"]);
				this->stepOffset->serialize(json["stepOffset"]);
			}

			//---------
			void
				Installation::deserialize(const nlohmann::json& json)
			{
				if (json.contains("columns")) {
					this->columns.deserialize(json["columns"]);
				}

				if (json.contains("stepOffset")) {
					this->stepOffset->deserialize(json["stepOffset"]);
				}

				this->initColumns();
			}

			//---------
			void
				Installation::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Build", [this]() {
					this->build();
					}, OF_KEY_RETURN);

				inspector->addSubMenu("Step offset", this->stepOffset);
			}

			//---------
			ofxCvGui::PanelPtr
				Installation::getPanel()
			{
				return this->panel;
			}

			//---------
			const
				Installation::PhysicalParameters&
				Installation::getPhysicalParameters() const
			{
				return this->parameters.physicalParameters;
			}

			//---------
			void
				Installation::build()
			{
				this->columns.clear();

				// Get the section step parameters
				auto sectionStepEnabled = this->parameters.builder.sectionStep.enabled.get();
				auto sectionSize = this->parameters.builder.sectionStep.sectionSize.get();

				// Get the main parameters
				auto countX = this->parameters.builder.basic.countX.get();
				auto countY = this->parameters.builder.basic.countY.get();
				auto pitch = this->parameters.builder.basic.pitch.get();

				auto transformEveryColumn = glm::translate(glm::vec3(pitch, 0, 0));
				auto transformEverySection = this->stepOffset->getTransform();

				// Create the columns to start with (reuse existing if already existing)
				this->columns.resize(countX, []() {return make_shared<Data::Reworld::Column>();  });
				auto allColumns = this->columns.getAllCaptures();;

				auto runningTransform = glm::translate(glm::vec3(pitch / 2.0f, 0.0f, 0.0f));

				for (int i = 0; i < countX; i++) {
					auto column = allColumns[i];

					// Column transform
					{
						// Section jumps (but not for first one)
						if (i != 0 && (i % sectionSize) == 0) {
							runningTransform = transformEverySection * runningTransform;
						}

						column->rigidBody->setTransform(runningTransform);

						// Apply running transform
						runningTransform = transformEveryColumn * runningTransform;
					}

					column->build(i, countY, pitch);
				}
			}

			//---------
			void
				Installation::initColumns()
			{
				// Sub selections
				{
					auto columns = this->columns.getAllCaptures();
					for (auto column : columns) {
						column->parentSelection = &this->ourSelection;
						column->setParent(this);
						column->init();
					}
				}
			}

			//---------
			vector<shared_ptr<Data::Reworld::Module>>
				Installation::getModules() const
			{
				vector<shared_ptr<Data::Reworld::Module>> modules;

				auto selectedColumns = this->columns.getSelection();
				for (auto column : selectedColumns) {
					const auto selectedModules = column->modules.getSelection();
					modules.insert(modules.end(), selectedModules.begin(), selectedModules.end());
				}

				return modules;
			}
		}
	}
}
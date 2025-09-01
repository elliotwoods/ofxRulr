#include "pch_Plugin_Reworld.h"
#include "Installation.h"
#include "ColumnView.h"
#include "Router.h"

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
					panel->addButton("Select all modules", [this]() {
						this->selectAllModules();
						});
					panel->addButton("Home and zero", [this]() {
						try {
							this->homeAndZero();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					panel->addButton("Goto see through", [this]() {
						try {
							this->gotoSeeThrogh();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					this->panel = panel;
				}

				this->addInput<Router>();
			}

			//---------
			void
				Installation::update()
			{
				auto columns = this->getAllColumns();

				// update all columns
				{

					for (auto column : columns) {
						column->update();
					}
				}

				// Send fresh data (either on period or change)
				if(this->getInput<Router>()) {
					bool sendAllValues = false;

					if (this->parameters.transmit.periodEnabled.get()) {
						auto period = this->parameters.transmit.onPeriod.get();
						auto currentTime = ofGetElapsedTimef();
						if (currentTime - this->transmit.lastSendTime > period) {
							this->transmit.lastSendTime = currentTime;
							sendAllValues = true;
						}
					}

					this->pushAllValues(sendAllValues, true);
				}
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

				inspector->addButton("Home and zero", [this]() {
					try {
						this->homeAndZero();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
				inspector->addButton("Goto see through", [this]() {
					try {
						this->gotoSeeThrogh();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});

				inspector->addButton("Export postiions CSV", [this]() {
					try {
						this->exportPositionsCSV();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
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
				Installation::setPhysicalParameters(const Installation::PhysicalParameters& newParameters)
			{
				this->parameters.physicalParameters.interPrismDistanceMM.set(newParameters.interPrismDistanceMM.get());
				this->parameters.physicalParameters.prismAngle.set(newParameters.prismAngle.get());
				this->parameters.physicalParameters.ior.set(newParameters.ior.get());
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
				this->initColumns();
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
			void
				Installation::selectAllModules()
			{
				auto columns = this->getAllColumns();
				for (auto column : columns) {
					column->setSelected(true);

					auto modules = column->getAllModules();
					for (auto module : modules) {
						module->setSelected(true);
					}
				}
			}

			//---------
			void
				Installation::pushAllValues(bool forceSend, bool selectedOnly)
			{
				auto columns = this->getAllColumns();

				this->throwIfMissingAConnection<Router>();
				auto router = this->getInput<Router>();

				bool sendOnChange = this->parameters.transmit.onChange.get();

				if (sendOnChange) {
					map<Router::Address, Models::Reworld::AxisAngles<float>> dataToSend;
					for (int columnIndex = 0; columnIndex < columns.size(); columnIndex++) {
						auto column = columns[columnIndex];

						if (!column->isSelected() && selectedOnly) {
							continue;
						}

						auto modules = column->getAllModules();
						for (int moduleIndex = 0; moduleIndex < modules.size(); moduleIndex++) {
							auto module = modules[moduleIndex];

							if (!module->isSelected() && selectedOnly) {
								continue;
							}

							// send value if needed
							bool sendThisValue = forceSend;
							sendThisValue |= sendOnChange && module->needsSendAxisAngles();

							if (sendThisValue) {
								Router::Address addressZeroIndexed;
								{
									addressZeroIndexed.column = columnIndex;
									addressZeroIndexed.portal = moduleIndex;
								}

								auto axisValues = module->getAxisAnglesForSend();
								dataToSend.emplace(addressZeroIndexed, axisValues);
							}

							// check if there's anything in the outbox
							auto moduleOutbox = module->getAndClearOSCOutbox();
							if (!moduleOutbox.empty()) {
								Router::Address addressTargetIndexed;
								{
									addressTargetIndexed.column = columnIndex;
									addressTargetIndexed.portal = module->parameters.ID.get();
								}

								for (auto item : moduleOutbox) {
									router->sendOSCMessageToModule(addressTargetIndexed, item);
								}
							}
						}

						// check if there's anything in the outbox
						auto columnOutbox = column->getAndClearOSCOutbox();
						if (!columnOutbox.empty()) {
							for (auto item : columnOutbox) {
								router->sendOSCMessageToColumn(columnIndex, item);
							}
						}
					}

					if (!dataToSend.empty()) {
						router->sendAxisValues(dataToSend);
					}
				}
			}

			//---------
			void
				Installation::homeAndZero()
			{
				auto columns = this->getAllColumns();
				for (auto column : columns) {
					auto modules = column->getAllModules();
					for (auto module : modules) {
						module->setTargetAxisAngles({ 0, 0.0 });
					}
				}
				
				auto router = this->getInput<Router>();
				router->sendOSCMessageToAll("home");
			}

			//---------
			void
				Installation::gotoSeeThrogh()
			{
				auto columns = this->getAllColumns();
				for (auto column : columns) {
					auto modules = column->getAllModules();
					for (auto module : modules) {
						module->setTargetAxisAngles({ 0.5, 0.0 });
					}
				}

				auto router = this->getInput<Router>();
				router->sendOSCMessageToAll("seeThrough");
			}

			//---------
			void
				Installation::exportPositionsCSV() const
			{
				auto result = ofSystemSaveDialog("module_positions.csv", "Save module positions");
				if (!result.bSuccess || result.getPath().empty()) {
					return;
				}

				std::ofstream ofs(result.getPath(), std::ios::out | std::ios::trunc);
				if (!ofs.is_open()) {
					ofLogError("Installation") << "Failed to open file for writing: " << result.getPath();
					return;
				}

				// Optional: write header
				// ofs << "columnIndex,moduleIndex,x,y,z\n";

				auto columns = this->getAllColumns();
				for (int columnIndex = 0; columnIndex < static_cast<int>(columns.size()); columnIndex++) {
					auto modules = columns[columnIndex]->getAllModules();
					for (int moduleIndex = 0; moduleIndex < static_cast<int>(modules.size()); moduleIndex++) {
						const auto& module = modules[moduleIndex];
						const auto position = module->getPosition();
						ofs << columnIndex << ","
							<< moduleIndex << ","
							<< std::setprecision(9) << std::fixed
							<< position.x << ","
							<< position.y << ","
							<< position.z << "\n";
					}
				}

				ofs.close();
			}

			//---------
			vector<shared_ptr<Data::Reworld::Module>>
				Installation::getSelectedModules() const
			{
				vector<shared_ptr<Data::Reworld::Module>> modules;

				auto selectedColumns = this->columns.getSelection();
				for (auto column : selectedColumns) {
					const auto selectedModules = column->modules.getSelection();
					modules.insert(modules.end(), selectedModules.begin(), selectedModules.end());
				}

				return modules;
			}

			//---------
			vector<shared_ptr<Data::Reworld::Column>>
				Installation::getAllColumns() const
			{
				return this->columns.getAllCaptures();
			}

			//---------
			shared_ptr<Data::Reworld::Column>
				Installation::getColumnByIndex(Data::Reworld::ColumnIndex columnIndex, bool selectedOnly) const
			{
				auto columns = this->getAllColumns();

				if (columnIndex >= columns.size() || columnIndex < 0) {
					throw(ofxRulr::Exception("Index " + ofToString(columnIndex) + " column out of bounds"));
				}

				auto column = columns[columnIndex];

				if (selectedOnly && !column->isSelected()) {
					return shared_ptr<Data::Reworld::Column>();
				}

				return column;
			}

			//---------
			shared_ptr<Data::Reworld::Module>
				Installation::getModuleByIndices(Data::Reworld::ColumnIndex columnIndex, Data::Reworld::ModuleIndex moduleIndex, bool selectedOnly) const
			{
				auto column = this->getColumnByIndex(columnIndex, selectedOnly);
				if (!column) {
					return shared_ptr<Data::Reworld::Module>();
				}
				return column->getModuleByIndex(moduleIndex, selectedOnly);
			}

			//---------
			map<Data::Reworld::ColumnIndex, shared_ptr<Data::Reworld::Column>>
				Installation::getSelectedColumnByIndex() const
			{
				map<Data::Reworld::ColumnIndex, shared_ptr<Data::Reworld::Column>> columnsByIndex;

				auto columns = this->getAllColumns();
				for (Data::Reworld::ColumnIndex columnIndex = 0; columnIndex < columns.size(); columnIndex++) {
					auto column = columns[columnIndex];
					if (column->isSelected()) {
						columnsByIndex.emplace(columnIndex, column);
					}
				}

				return columnsByIndex;
			}

			//---------
			map<Data::Reworld::ColumnIndex, map<Data::Reworld::ModuleIndex, shared_ptr<Data::Reworld::Module>>>
				Installation::getSelectedModulesByIndex() const
			{
				map<Data::Reworld::ColumnIndex, map<Data::Reworld::ModuleIndex, shared_ptr<Data::Reworld::Module>>> modulesByIndex;

				auto columnsByIndex = this->getSelectedColumnByIndex();
				for (auto columnIt : columnsByIndex) {
					modulesByIndex.emplace(columnIt.first, columnIt.second->getSelectedModulesByIndex());
				}

				return modulesByIndex;
			}
		}
	}
}
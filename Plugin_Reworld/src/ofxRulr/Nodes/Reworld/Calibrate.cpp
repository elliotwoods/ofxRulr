#include "pch_Plugin_Reworld.h"
#include "Calibrate.h"
#include "Installation.h"
#include "CalibrateController.h"

#include "ofxRulr/Solvers/Reworld/Calibrate/ModulesFromProjections.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//----------
			Calibrate::CalibrateControllerSession::CalibrateControllerSession()
			{
				static int controllerIndex = 0;
				ofColor color(255, 150, 150);
				color.setHueAngle(40 * controllerIndex++);
				this->color = color;
			}

			//----------
			Data::Reworld::ColumnIndex
				Calibrate::CalibrateControllerSession::getColumnIndex() const
			{
				return this->columnIndex;
			}

			//----------
			Data::Reworld::ModuleIndex
				Calibrate::CalibrateControllerSession::getModuleIndex() const
			{
				return this->moduleIndex;
			}

			//----------
			void
				Calibrate::CalibrateControllerSession::setColumnIndex(Data::Reworld::ColumnIndex columnIndex)
			{
				this->columnIndex = columnIndex;
				this->onSelectedModuleChange.notifyListeners();
			}

			//----------
			void
				Calibrate::CalibrateControllerSession::setModuleIndex(Data::Reworld::ModuleIndex moduleIndex)
			{
				this->moduleIndex = moduleIndex;
				this->onSelectedModuleChange.notifyListeners();
			}

			//----------
			ofColor
				Calibrate::CalibrateControllerSession::getColor() const
			{
				return this->color;
			}


			//----------
			void
				Calibrate::CalibrateControllerSession::setName(const string& name)
			{
				this->name = name;
				this->onSelectedModuleChange.notifyListeners();
			}


			//----------
			string
				Calibrate::CalibrateControllerSession::getName() const
			{
				return this->name;
			}

			//----------
			Calibrate::Calibrate()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Calibrate::getTypeName() const
			{
				return "Reworld::Calibrate";
			}

			//----------
			void
				Calibrate::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<Installation>();
				this->addInput<Item::RigidBody>("Light position");

				{
					this->panel = ofxCvGui::Panels::makeWidgets();
					{
						auto button = this->panel->addToggle(this->parameters.control.controlOn);
						button->setHeight(100.0f);
					}

					this->captures.populateWidgets(this->panel);
					this->panel->addButton("New capture", [this]() {
						this->newCapture();
						});
				}

				{
					this->parameters.control.estimation.solverSettings.printEachStep.set(false);
					this->parameters.control.estimation.solverSettings.printReport.set(false);
				}

				this->ourSelection.onSelectionChanged += [this]() {
					this->needsLoadCaptureData = true;
					this->needsCalculateParking = true;
					};

				this->manageParameters(this->parameters);
			}

			//----------
			void
				Calibrate::update()
			{
				// Control
				if (this->parameters.control.controlOn.get()) {
					// Load capture data
					if (this->needsLoadCaptureData) {
						if (this->ourSelection.selection) {
							try {
								this->moveToCapturePositions(this->ourSelection.selection);
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
						this->needsLoadCaptureData = false;
					}

					// Parking
					if (this->needsCalculateParking) {
						this->calculateParking();
					}
				}

				// update calibrate controller session related caches
				if (this->calibrateControllerSessionDataChanged) {
					{
						map<Router::Address, vector<string>> cache;
						for (auto it : this->calibrateControllerSessions) {
							auto columnIndex = it.second->getColumnIndex();
							auto moduleIndex = it.second->getModuleIndex();
							Router::Address address{
								columnIndex
								, (uint8_t)moduleIndex
							};
							cache[address].push_back(it.second->getName());
						}
						this->calibrateControllerSessionsNameCache = cache;
					}

					this->needsCalculateParking = true;
					this->calibrateControllerSessionDataChanged = false;
				}

				// update selected module indices cache (modules selected in installation)
				{
					this->selectedModuleIndicesCache.clear();
					auto installation = this->getInput<Installation>();
					if (installation) {
						auto columns = installation->getAllColumns();
						for (int columnIndex = 0; columnIndex < columns.size(); columnIndex++) {
							auto column = columns[columnIndex];

							if (!column->isSelected()) {
								continue;
							}

							auto modules = column->getAllModules();
							for (int moduleIndex = 0; moduleIndex < modules.size(); moduleIndex++) {
								auto module = modules[moduleIndex];
								if (module->isSelected()) {
									Router::Address address;
									{
										address.column = columnIndex;
										address.portal = (uint8_t)moduleIndex;
									}
									this->selectedModuleIndicesCache.emplace(address);
								}
							}
						}
					}
				}
			}

			//----------
			void
				Calibrate::drawWorldStage()
			{
				// Draw selections
				{
					auto installation = this->getInput<Installation>();
					if (!installation) return;

					for (auto it : this->calibrateControllerSessions) {
						auto name = it.first->getName();
						try {
							auto module = installation->getModuleByIndices(it.second->getColumnIndex(), it.second->getModuleIndex(), false);
							ofxCvGui::Utils::drawTextAnnotation(name, module->getPosition(), it.second->getColor());
						}
						catch (...) {

						}
					}
				}

				// Draw captures
				{
					auto captures = this->captures.getSelection();
					for (auto capture : captures) {
						capture->drawWorldStage();
					}
				}

			}

			//----------
			ofxCvGui::PanelPtr
				Calibrate::getPanel()
			{
				return this->panel;
			}


			//----------
			void
				Calibrate::serialize(nlohmann::json& json)
			{
				this->captures.serialize(json["captures"]);
			}

			//----------
			void
				Calibrate::deserialize(const nlohmann::json& json)
			{
				if (json.contains("captures")) {
					this->captures.deserialize(json["captures"]);
				}
				this->initCaptures();
			}

			//----------
			void
				Calibrate::populateInspector(ofxCvGui::InspectArguments args)
			{
				auto inspector = args.inspector;

				inspector->addButton("Calculate parking", [this]() {
					this->calculateParking();
					});

				inspector->addButton("Calibrate", [this]() {
					try {
						this->calibrate();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

				inspector->addButton("Calculate residuals", [this]() {
					try {
						this->calculateResiduals();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});

				inspector->addButton("Clear per module calibrations", [this]() {
					try {
						this->clearPerModuleCalibrations();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
			}

			//----------
			shared_ptr<Calibrate::CalibrateControllerSession>
				Calibrate::registerCalibrateController(CalibrateController* calibrateController)
			{
				// just in case
				this->unregisterCalibrateController(calibrateController);

				auto calibrateControllerSession = make_shared<CalibrateControllerSession>();
				calibrateControllerSession->onSelectedModuleChange += [this]() {
					this->calibrateControllerSessionDataChanged = true;
					};
				this->calibrateControllerSessions.emplace(calibrateController, calibrateControllerSession);

				return calibrateControllerSession;
			}

			//----------
			void
				Calibrate::unregisterCalibrateController(CalibrateController* calibrateController)
			{
				auto it = this->calibrateControllerSessions.find(calibrateController);

				if (it != this->calibrateControllerSessions.end()) {
					this->calibrateControllerSessions.erase(it);
				}

			}
			//----------
			vector<shared_ptr<Calibrate::CalibrateControllerSession>>
				Calibrate::getCalibrateControllerSessions() const
			{
				vector<shared_ptr<CalibrateControllerSession>> calibrateControllerSessions;
				for (auto it : this->calibrateControllerSessions) {
					calibrateControllerSessions.push_back(it.second);
				}
				return calibrateControllerSessions;
			}
			
			//----------
			map<Router::Address, vector<string>>
				Calibrate::getCalibrateControllerSessionsNameCache() const
			{
				return this->calibrateControllerSessionsNameCache;
			}

			//----------
			const set<Router::Address>&
				Calibrate::getSelectedModuleIndicesCache() const
			{
				return this->selectedModuleIndicesCache;
			}

			//----------
			void
				Calibrate::initCaptures()
			{
				// Sub selections
				{
					auto captures = this->captures.getAllCaptures();
					for (auto capture : captures) {
						this->initCapture(capture);
					}
				}
			}

			//----------
			void
				Calibrate::initCapture(shared_ptr<Data::Reworld::Capture> capture)
			{
				capture->parentSelection = &this->ourSelection;
				capture->setParent(this);
				capture->init();

				capture->onModuleDataPointsChange.clear();
				capture->onModuleDataPointsChange += [this]() {
					this->needsCalculateParking = true;
					};
			}

			//----------
			shared_ptr<Data::Reworld::Capture>
				Calibrate::newCapture()
			{
				auto capture = make_shared<Data::Reworld::Capture>();
				this->initCapture(capture);
				this->captures.add(capture);
				return capture;
			}

			//----------
			void
				Calibrate::estimateModuleData(Data::Reworld::Capture* capture)
			{
				this->throwIfMissingAnyConnection();
				auto installation = this->getInput<Installation>();

				const auto filteringEnabled = this->parameters.control.estimation.filterModules.enabled.get();
				if (filteringEnabled) {
					// start with everything selected if we are filtering
					installation->selectAllModules();
				}

				auto light = this->getInput<Item::RigidBody>();
				auto lightPosition = light->getPosition();

				if (!capture->getTargetIsSet()) {
					throw(Exception("No target is set for this capture yet"));
				}

				auto columns = installation->getAllColumns();
				for (int i = 0; i < columns.size(); i++) {
					const auto& column = columns[i];
					const auto modules = column->getAllModules();
					for (int j = 0; j < modules.size(); j++) {
						auto module = modules[j];
						auto result = Solvers::Reworld::Navigate::PointToPoint::solve(module->getModel()
							, module->getCurrentAxisAngles()
							, lightPosition
							, capture->getTarget());

						capture->setModuleDataEstimate(i, j, result.solution.axisAngles);

						if (filteringEnabled) {
							// Check if we're outside acceptable range and deselect the module

							auto vector = Models::Reworld::axisAnglesToVector(result.solution.axisAngles);
							auto r = glm::length(vector);

							if (result.residual > this->parameters.control.estimation.filterModules.maxResidual.get()
								|| r < this->parameters.control.estimation.filterModules.deadZone.get()) {
								// if the module lands too close to the see-through singularity it will be impossible to calibrate
								module->setSelected(false);
							}
						}
					}
				}

				this->moveToCapturePositions(capture);
			}

			//----------
			void
				Calibrate::moveToCapturePositions(Data::Reworld::Capture* capture)
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				for (const auto& columnIt : capture->moduleDataPoints) {
					for (const auto& moduleIt : columnIt.second) {
						auto module = installation->getModuleByIndices(columnIt.first, moduleIt.first, false);
						if (moduleIt.second->state != Data::Reworld::Capture::ModuleDataPoint::Unset) {
							module->setTargetAxisAngles(moduleIt.second->axisAngles);
						}
					}
				}
			}

			//----------
			void
				Calibrate::moveDataPoint(shared_ptr<CalibrateControllerSession> calibrateControllerSession, const glm::vec2& movement)
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				auto module = installation->getModuleByIndices(calibrateControllerSession->getColumnIndex(), calibrateControllerSession->getModuleIndex(), true);
				if (!module) {
					throw(Exception("moveDataPoint - module is not available (e.g. not selected)"));
				}
				auto currentAxisAngles = module->getCurrentAxisAngles();
				auto currentPolar = Models::Reworld::axisAnglesToPolar(currentAxisAngles);
				auto currentVector = Models::Reworld::polarToVector(currentPolar);

				auto newVector = currentVector + movement;

				// Clamp to max length of 1
				auto newVectorLength = glm::length(newVector);
				if (newVectorLength > 1) {
					newVector /= newVectorLength;
				}

				auto newPolar = Models::Reworld::vectorToPolar(newVector);
				auto newAxisAngles = Models::Reworld::polarToAxisAngles(newPolar);

				// Send it to the module
				module->setTargetAxisAngles(newAxisAngles);

				// Save it to the current capture
				auto capture = ourSelection.selection;
				if (capture) {
					capture->setManualModuleData(calibrateControllerSession->getColumnIndex()
						, calibrateControllerSession->getModuleIndex(), newAxisAngles);
				}
			}

			//----------
			void
				Calibrate::markDataPointGood(shared_ptr<CalibrateControllerSession> calibrateControllerSession)
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				auto module = installation->getModuleByIndices(calibrateControllerSession->getColumnIndex(), calibrateControllerSession->getModuleIndex(), true);
				if (!module) {
					throw(Exception("moveDataPoint - module is not available (e.g. not selected)"));
				}

				// Save it to the current capture
				auto capture = ourSelection.selection;
				if (!capture) {
					throw(Exception("No capture selected"));
				}
				capture->markDataPointGood(calibrateControllerSession->getColumnIndex()
					, calibrateControllerSession->getModuleIndex());
			}

			//----------
			void
				Calibrate::clearModuleDataSetValues(shared_ptr<CalibrateControllerSession> calibrateControllerSession)
			{
				auto capture = ourSelection.selection;
				if (!capture) {
					throw(Exception("No capture selected"));
				}
				capture->clearModuleDataSetValues(calibrateControllerSession->getColumnIndex()
					, calibrateControllerSession->getModuleIndex());
			}

			//----------
			void
				Calibrate::calculateParking()
			{
				auto selectedCapture = this->ourSelection.selection;
				if (!selectedCapture) {
					return;
				}

				auto installation = this->getInput<Installation>();
				if (!installation) {
					return;
				}

				bool parkingEnabled = this->parameters.control.parkNonSelected.enabled.get();

				// Perform parking
				for (const auto& columnDataPointsIt : selectedCapture->moduleDataPoints) {
					for (const auto& moduleDataPointIt : columnDataPointsIt.second) {
						auto axisAngles = moduleDataPointIt.second->axisAngles;
						auto module = installation->getModuleByIndices(columnDataPointsIt.first, moduleDataPointIt.first, false);
						auto& moduleDataPoint = moduleDataPointIt.second;

						if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Unset) {
							// Don't do anything if there's no data at all
							continue;
						}


						Router::Address address;
						{
							address.portal = moduleDataPointIt.first;
							address.column = columnDataPointsIt.first;
						}

						auto isSelectedByAController = this->calibrateControllerSessionsNameCache.find(address) != this->calibrateControllerSessionsNameCache.end();

						if (parkingEnabled && !isSelectedByAController) {
							// calculate an offset for the position (a parked position)
							float offset = 0.0f;
							if (!module->isSelected()) {
								// unselected
								offset = this->parameters.control.parkNonSelected.parkingSpots.unselected.get();
							}
							else if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Estimated) {
								offset = this->parameters.control.parkNonSelected.parkingSpots.estimated.get();
							}
							else if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Set) {
								offset = this->parameters.control.parkNonSelected.parkingSpots.set.get();
							}
							else if (moduleDataPoint->state == Data::Reworld::Capture::ModuleDataPoint::Good) {
								offset = this->parameters.control.parkNonSelected.parkingSpots.good.get();
							}

							axisAngles.A += offset;
							axisAngles.B += offset;
						}

						module->setTargetAxisAngles(axisAngles);
					}
				}

				needsCalculateParking = false;
			}

			//----------
			void
				Calibrate::gatherData(GatheredData& data) const
			{
				data.modules.clear();
				data.flatToIndexed.clear();
				data.captures.clear();
				data.targetPositions.clear();

				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				// modules + flat index mapping
				{
					auto modulesPerColumn = installation->getSelectedModulesByIndex();
					for (const auto& [columnIdx, perColumn] : modulesPerColumn) {
						for (const auto& [moduleIdx, modulePtr] : perColumn) {
							if (modulePtr) {
								data.flatToIndexed.emplace_back(columnIdx, moduleIdx);
								data.modules.emplace_back(modulePtr);
							}
						}
					}
				}

				// captures + targets
				{
					auto capturesSelection = this->captures.getSelection();
					data.targetPositions.reserve(capturesSelection.size());
					for (const auto& capture : capturesSelection) {
						if (capture && capture->getTargetIsSet()) {
							data.captures.emplace_back(capture);
							data.targetPositions.emplace_back(capture->getTarget());
						}
					}
				}
			}

			//----------
			void
				Calibrate::calibrate()
			{
				// Get the light position
				auto lightNode = this->getInput<Item::RigidBody>("Light position");
				auto lightPosition = lightNode->getPosition();

				// Get the installation
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				// Gather shared data
				GatheredData data;
				this->gatherData(data);

				// Enforce minimum datapoints per module (count only points with state >= Good)
				{
					vector<size_t> goodCountPerFlat(data.modules.size(), 0);

					for (const auto& capture : data.captures) {
						for (size_t flatIdx = 0; flatIdx < data.flatToIndexed.size(); flatIdx++) {
							const auto& [columnIdx, moduleIdx] = data.flatToIndexed[flatIdx];
							auto dp = capture->getModuleDataPoint(columnIdx, moduleIdx, false);
							if (!dp) {
								continue;
							}
							if ((int)dp->state >= (int)Data::Reworld::Capture::ModuleDataPoint::State::Good) {
								goodCountPerFlat[flatIdx]++;
							}
						}
					}

					int minPts = this->parameters.solve.minDataPointsPerModule.get();
					if (!this->parameters.solve.fixAllModulePositions.get()
						|| !this->parameters.solve.fixAllModuleRotations.get()
						|| !this->parameters.solve.fixAllAxisAngleOffsets.get())
					{
						// If we're changing module transforms then we need at least 2 points (maybe 3 really)
						if (minPts < 2) {
							minPts = 2;
						}
					}

					vector<shared_ptr<Data::Reworld::Module>> filteredModules;
					vector<pair<Data::Reworld::ColumnIndex, Data::Reworld::ModuleIndex>> filteredFlatToIndexed;

					for (size_t flatIdx = 0; flatIdx < data.modules.size(); flatIdx++) {
						if ((int)goodCountPerFlat[flatIdx] >= minPts) {
							filteredModules.emplace_back(data.modules[flatIdx]);
							filteredFlatToIndexed.emplace_back(data.flatToIndexed[flatIdx]);
						}
					}

					data.modules.swap(filteredModules);
					data.flatToIndexed.swap(filteredFlatToIndexed);

					if (data.modules.empty()) {
						// Nothing to solve
						this->parameters.solve.solveResidual.set(0.0f);
						return;
					}
				}

				// Build initial solution from current state
				Solvers::Reworld::Calibrate::ModuleFromProjections::Solution initialSolution;
				{
					// Globals
					initialSolution.lightPosition = lightPosition;

					const auto physical = installation->getPhysicalParameters();
					initialSolution.interPrismDistance = physical.interPrismDistanceMM.get() * 0.001f; // mm -> m
					initialSolution.prismAngleRadians = physical.prismAngle.get() * (float)DEG_TO_RAD; // deg -> rad
					initialSolution.ior = physical.ior.get();

					// Per-module
					initialSolution.modules.reserve(data.modules.size());
					for (const auto& m : data.modules) {
						initialSolution.modules.emplace_back(m->getModel());
					}
				}

				// Create problem
				Solvers::Reworld::Calibrate::ModuleFromProjections::Problem problem(initialSolution, data.targetPositions);

				// Add observations (only datapoints with state >= Good)
				{
					for (size_t targetIdx = 0; targetIdx < data.captures.size(); targetIdx++) {
						const auto& capture = data.captures[targetIdx];

						for (size_t flatIdx = 0; flatIdx < data.flatToIndexed.size(); flatIdx++) {
							const auto& [columnIdx, moduleIdx] = data.flatToIndexed[flatIdx];

							auto dp = capture->getModuleDataPoint(columnIdx, moduleIdx, false);
							if (!dp) {
								continue;
							}
							if ((int)dp->state < (int)Data::Reworld::Capture::ModuleDataPoint::State::Good) {
								continue; // skip unless Good (or higher if you add states later)
							}

							problem.addProjectionObservation((int)flatIdx, (int)targetIdx, dp->axisAngles);
						}
					}
				}

				// Set which parameters are fixed/variable
				{
					if (this->parameters.solve.fixLightPosition.get()) {
						problem.setLightPositionFixed();
					}
					else {
						problem.setLightPositionVariable();
					}

					if (this->parameters.solve.fixInterPrismDistance.get()) {
						problem.setInterPrismDistanceFixed();
					}
					else {
						problem.setInterPrismDistanceVariable();
					}

					if (this->parameters.solve.fixPrismAngle.get()) {
						problem.setPrismAngleFixed();
					}
					else {
						problem.setPrismAngleVariable();
					}

					if (this->parameters.solve.fixIOR.get()) {
						problem.setIORFixed();
					}
					else {
						problem.setIORVariable();
					}

					if (this->parameters.solve.fixAllModulePositions.get()) {
						problem.setAllModulePositionsFixed();
					}
					else {
						problem.setAllModulePositionsVariable();
					}

					if (this->parameters.solve.fixAllModuleRotations.get()) {
						problem.setAllModuleRotationsFixed();
					}
					else {
						problem.setAllModuleRotationsVariable();
					}

					if (this->parameters.solve.fixAllAxisAngleOffsets.get()) {
						problem.setAllAxisAngleOffsetsFixed();
					}
					else {
						problem.setAllAxisAngleOffsetsVariable();
					}
				}

				// Solve
				auto solverSettings = this->parameters.solve.solverSettings.getSolverSettings();
				{
					solverSettings.options.linear_solver_type = ceres::LinearSolverType::DENSE_SCHUR;
				}
				Solvers::Reworld::Calibrate::ModuleFromProjections::Result result =
					problem.solve(solverSettings);

				// Write back results
				{
					// Light
					lightNode->setPosition(result.solution.lightPosition);

					// Physical parameters
					{
						auto physical = installation->getPhysicalParameters();
						physical.interPrismDistanceMM.set(result.solution.interPrismDistance * 1000.0f);     // m -> mm
						physical.prismAngle.set(result.solution.prismAngleRadians * (float)RAD_TO_DEG);      // rad -> deg
						physical.ior.set(result.solution.ior);
						installation->setPhysicalParameters(physical);
					}

					// Modules (order matches filtered data.modules)
					{
						const auto& solvedModules = result.solution.modules;
						const size_t n = std::min(solvedModules.size(), data.modules.size());
						for (size_t i = 0; i < n; i++) {
							const auto& solved = solvedModules[i];
							auto& mod = data.modules[i];

							// Transform offset
							mod->parameters.calibrationParameters.transformOffset.translation.x.set(solved.transformOffset.translation.x);
							mod->parameters.calibrationParameters.transformOffset.translation.y.set(solved.transformOffset.translation.y);
							mod->parameters.calibrationParameters.transformOffset.translation.z.set(solved.transformOffset.translation.z);

							mod->parameters.calibrationParameters.transformOffset.rotationVector.x.set(solved.transformOffset.rotationVector.x);
							mod->parameters.calibrationParameters.transformOffset.rotationVector.y.set(solved.transformOffset.rotationVector.y);
							mod->parameters.calibrationParameters.transformOffset.rotationVector.z.set(solved.transformOffset.rotationVector.z);

							// Axis-angle offsets
							mod->parameters.calibrationParameters.axisAngleOffsets.A.set(solved.axisAngleOffsets.A);
							mod->parameters.calibrationParameters.axisAngleOffsets.B.set(solved.axisAngleOffsets.B);
						}
					}
				}

				// Residuals from *current stored values*
				this->calculateResiduals();
			}

			//----------
			void
				Calibrate::calculateResiduals()
			{
				// Build shared data (no filtering by min datapoints here)
				GatheredData data;
				this->gatherData(data);

				// Build a Solution from current stored values
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();

				Solvers::Reworld::Calibrate::ModuleFromProjections::Solution currentSolution;
				{
					// Light
					{
						auto lightNode = this->getInput<Item::RigidBody>("Light position");
						currentSolution.lightPosition = lightNode->getPosition();
					}

					// Physical
					{
						const auto physical = installation->getPhysicalParameters();
						currentSolution.interPrismDistance = physical.interPrismDistanceMM.get() * 0.001f;   // mm -> m
						currentSolution.prismAngleRadians = physical.prismAngle.get() * (float)DEG_TO_RAD;   // deg -> rad
						currentSolution.ior = physical.ior.get();
					}

					// Modules (as stored)
					currentSolution.modules.reserve(data.modules.size());
					for (const auto& m : data.modules) {
						currentSolution.modules.emplace_back(m->getModel());
					}
				}

				// Compute per-datapoint residuals and overall RMS
				double sumSq = 0.0;
				size_t count = 0;

				for (size_t targetIdx = 0; targetIdx < data.captures.size(); targetIdx++) {
					const auto& capture = data.captures[targetIdx];
					bool anyChanged = false;

					for (size_t flatIdx = 0; flatIdx < data.flatToIndexed.size(); flatIdx++) {
						const auto& [columnIdx, moduleIdx] = data.flatToIndexed[flatIdx];

						auto dp = capture->getModuleDataPoint(columnIdx, moduleIdx, false);
						if (!dp) {
							continue;
						}

						// For residuals, accept any datapoint with state >= Estimated
						if ((int)dp->state < (int)Data::Reworld::Capture::ModuleDataPoint::State::Estimated) {
							continue;
						}

						// Guard
						if (flatIdx >= currentSolution.modules.size()) {
							continue;
						}

						const auto& moduleModel = currentSolution.modules[flatIdx];
						const auto& target = capture->getTarget();

						// Choose axis angles: prefer set/good, else estimate (state>=Estimated guarantees availability)
						Models::Reworld::AxisAngles<float> axisAngles;
						if ((int)dp->state >= (int)Data::Reworld::Capture::ModuleDataPoint::State::Set) {
							axisAngles = dp->axisAngles;
						}
						else {
							axisAngles = dp->estimatedAxisAngles;
						}

						const float rms = Solvers::Reworld::Calibrate::ModuleFromProjections::getResidual(
							currentSolution
							, moduleModel
							, target
							, axisAngles);

						dp->residual = rms;
						sumSq += rms * rms;
						count++;
						anyChanged = true;
					}

					if (anyChanged) {
						capture->onModuleDataPointsChange.notifyListeners();
						capture->columnsPanelDirty = true;
					}
				}

				// overall RMS
				if (count > 0) {
					this->parameters.solve.solveResidual.set((float)sqrt(sumSq / (double)count));
				}
				else {
					this->parameters.solve.solveResidual.set(0.0f);
				}
			}

			//----------
			void
				Calibrate::clearPerModuleCalibrations()
			{
				this->throwIfMissingAConnection<Installation>();
				auto installation = this->getInput<Installation>();
				
				auto columns = installation->getAllColumns();
				for (auto column : columns) {
					if (column->isSelected()) {
						auto modules = column->getAllModules();
						for (auto module : modules) {
							if (module->isSelected()) {
								module->clearCalibration();
							}
						}
					}
				}
			}
		}
	}
}
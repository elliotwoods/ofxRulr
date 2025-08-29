#include "pch_Plugin_Reworld.h"
#include "Capture.h"
#include "ofxRulr/Nodes/Reworld/Calibrate.h"

namespace ofxRulr {
	namespace Data {
		namespace Reworld {
			//----------
			void
				Capture::ModuleDataPoint::serialize(nlohmann::json& json)
			{
				// store state as integer
				json["state"] = static_cast<int>(this->state);

				// store axisAngles as array [A, B]
				json["axisAngles"] = { this->axisAngles.A, this->axisAngles.B };
			}

			//----------
			void
				Capture::ModuleDataPoint::deserialize(const nlohmann::json& json)
			{
				// state
				if (json.contains("state") && json["state"].is_number_integer()) {
					int stateInt = json["state"].get<int>();
					this->state = static_cast<State>(stateInt);
				}

				// axisAngles
				if (json.contains("axisAngles") && json["axisAngles"].is_array() && json["axisAngles"].size() == 2) {
					this->axisAngles.A = json["axisAngles"][0].get<float>();
					this->axisAngles.B = json["axisAngles"][1].get<float>();
				}
			}

			//----------
			Capture::Capture()
			{
				RULR_NODE_SERIALIZATION_LISTENERS;
			}

			//----------
			string
				Capture::getTypeName() const
			{
				return "Capture";
			}

			//----------
			string
				Capture::getDisplayString() const
			{
				return "";
			}

			//----------
			void
				Capture::setParent(Nodes::Reworld::Calibrate* parent)
			{
				this->parent = parent;
			}

			//----------
			void
				Capture::init()
			{
				this->columnsPanel = make_shared<ofxCvGui::Panels::Groups::Strip>();
			}

			//----------
			void
				Capture::serialize(nlohmann::json& json)
			{
				Utils::serialize(json["target"], this->target);
				Utils::serialize(json["comment"], this->comment);
				Utils::serialize(json["targetIsSet"], this->targetIsSet);

				{
					auto& jsonColumns = json["columns"];
					for (const auto& columnIt : this->moduleDataPoints) {
						auto columnIndex = columnIt.first;
						auto& jsonColumn = jsonColumns[ofToString(columnIndex)];
						const auto& columnData = columnIt.second;
						for (const auto& moduleIt : columnData) {
							auto moduleIndex = moduleIt.first;
							auto& jsonDataPoint = jsonColumn[ofToString(moduleIndex)];
							auto& dataPoint = moduleIt.second;

							dataPoint->serialize(jsonDataPoint);
						}
					}
				}
			}

			//----------
			void
				Capture::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, "target", this->target);
				Utils::deserialize(json, "comment", this->comment);
				Utils::deserialize(json, "targetIsSet", this->targetIsSet);

				this->moduleDataPoints.clear();

				if (!json.contains("columns") || !json["columns"].is_object()) {
					return; // nothing to load
				}

				const auto& jsonColumns = json["columns"];

				// iterate columns
				for (const auto& [columnKey, jsonColumn] : jsonColumns.items()) {
					const ColumnIndex columnIndex = static_cast<ColumnIndex>(ofToInt(columnKey));
					if (!jsonColumn.is_object()) {
						continue;
					}

					auto& columnMap = this->moduleDataPoints[columnIndex];

					// iterate modules
					for (const auto& [moduleKey, jsonDataPoint] : jsonColumn.items()) {
						const ModuleIndex moduleIndex = static_cast<ModuleIndex>(ofToInt(moduleKey));

						if (jsonDataPoint.is_null()) {
							// choice: skip or explicitly store a nullptr
							// columnMap[moduleIndex] = nullptr;
							continue;
						}

						auto dp = std::make_shared<ModuleDataPoint>();
						dp->deserialize(jsonDataPoint);
						columnMap[moduleIndex] = std::move(dp);
					}
				}
			}

			//----------
			void
				Capture::drawWorldStage()
			{
				if (this->getTargetIsSet()) {
					auto color = this->parentSelection->isSelected(this)
						? ofxCvGui::Utils::getBeatingSelectionColor()
						: this->color.get();

					ofxCvGui::Utils::drawTextAnnotation(this->getText(), this->target.get(), color);

					ofPushStyle();
					{
						ofSetColor(color);
						ofDrawSphere(this->target.get(), 0.02f);
					}
					ofPopStyle();
				}
			}

			//----------
			void
				Capture::updatePanel()
			{
				if (this->columnsPanelDirty) {
					this->rebuildColumnsPanel();
				}
			}

			//----------
			void
				Capture::setTarget(const glm::vec3& target)
			{
				this->target.set(target);
				this->targetIsSet = true;
			}

			//----------
			const glm::vec3&
				Capture::getTarget() const
			{
				return this->target.get();
			}

			//----------
			bool
				Capture::getTargetIsSet() const
			{
				return this->targetIsSet;
			}

			//----------
			void
				Capture::initialiseModuleDataWithEstimate(ColumnIndex columnIndex, ModuleIndex moduleIndex, const Module::AxisAngles& data)
			{
				auto moduleData = this->getModuleDataPoint(columnIndex, moduleIndex);
				moduleData->axisAngles = data;
				moduleData->state = ModuleDataPoint::State::Estimated;
			}

			//----------
			void
				Capture::setManualModuleData(ColumnIndex columnIndex, ModuleIndex moduleIndex, const Module::AxisAngles& data)
			{
				auto moduleData = this->getModuleDataPoint(columnIndex, moduleIndex);
				moduleData->axisAngles = data;
				moduleData->state = ModuleDataPoint::State::Set;
			}

			//----------
			void
				Capture::markDataPointGood(ColumnIndex columnIndex, ModuleIndex moduleIndex)
			{
				auto moduleData = this->getModuleDataPoint(columnIndex, moduleIndex);
				if (moduleData->state == ModuleDataPoint::Unset) {
					throw(Exception("Cannot mark a data point as good if it is not set or estimated yet"));
				}
				moduleData->state = ModuleDataPoint::Good;
			}

			//----------
			shared_ptr<Capture::ModuleDataPoint>
				Capture::getModuleDataPoint(ColumnIndex columnIndex, ModuleIndex moduleIndex)
			{
				bool newDataPoint = false;

				auto findColumn = this->moduleDataPoints.find(columnIndex);
				if (findColumn == this->moduleDataPoints.end()) {
					newDataPoint = true;
					this->moduleDataPoints.emplace(columnIndex, map<ModuleIndex, shared_ptr<ModuleDataPoint>>());
				}

				auto& columnData = this->moduleDataPoints[columnIndex];
				auto findModule = columnData.find(moduleIndex);
				if (findModule == columnData.end()) {
					newDataPoint = true;
					columnData.emplace(moduleIndex, make_shared<ModuleDataPoint>());
				}

				if (newDataPoint) {
					this->columnsPanelDirty = true;
				}

				return columnData[moduleIndex];
			}

			//----------
			void
				Capture::populatePanel(shared_ptr<ofxCvGui::Panels::Groups::Strip> panel)
			{
				panel->setDirection(ofxCvGui::Panels::Groups::Strip::Horizontal);

				{
					auto widgetsPanel = ofxCvGui::Panels::makeWidgets();
					widgetsPanel->addEditableValue<glm::vec3>("Target", [this]() {
						return this->target.get();
						}
						, [this](const string & valueString) {
							if (!valueString.empty()) {
								stringstream ss(valueString);
								glm::vec3 value;
								ss >> value;
								this->setTarget(value);
							}
							});
					widgetsPanel->addIndicatorBool("Target is set", [this]() {
						return this->targetIsSet;
						});
					widgetsPanel->addButton("Estimate values", [this]() {
						try {
							this->parent->estimateModuleData(this);
						}
						RULR_CATCH_ALL_TO_ERROR;
						});
					panel->add(widgetsPanel);
				}

				{
					panel->add(this->columnsPanel);
				}

				panel->setCellSizes({ 150, -1 });
			}

			//----------
			string
				Capture::getText() const
			{
				auto comment = this->comment.get();
				if (comment.empty()) {
					return this->timeString;
				}
				else {
					return comment;
				}
			}

			//----------
			void
				Capture::rebuildColumnsPanel()
			{
				this->columnsPanel->clear();

				for (const auto& itColumn : this->moduleDataPoints) {
					auto columnPanel = make_shared<ofxCvGui::Panels::Groups::Strip>();
					columnPanel->setScissorEnabled(false); // optimisation
					columnPanel->setDirection(ofxCvGui::Panels::Groups::Strip::Vertical);

					auto& columnData = itColumn.second;

					for (auto itModule = columnData.rbegin(); itModule != columnData.rend(); itModule++) {
						auto panel = ofxCvGui::Panels::makeBlank();
						auto columnIndex = itColumn.first;
						auto moduleIndex = itModule->first;

						panel->onDraw += [this, columnIndex, moduleIndex](ofxCvGui::DrawArguments& args) {
							//find the data if it exists
							auto findColumn = this->moduleDataPoints.find(columnIndex);
							if (findColumn == this->moduleDataPoints.end()) {
								return;
							}
							auto findModule = findColumn->second.find(moduleIndex);
							if (findModule == findColumn->second.end()) {
								return;
							}

							const auto& data = findModule->second;

							// Draw background based on state
							switch (data->state) {
							case ModuleDataPoint::State::Estimated:
								ofPushStyle();
								{
									ofSetColor(100);
									ofFill();
									ofDrawRectangle(args.localBounds);
								}
								ofPopStyle();
								break;
							case ModuleDataPoint::State::Set:
								ofPushStyle();
								{
									ofSetColor(200, 100, 100);
									ofFill();
									ofDrawRectangle(args.localBounds);
								}
								ofPopStyle();
								break;
							case ModuleDataPoint::State::Good:
								ofPushStyle();
								{
									ofSetColor(100, 200, 100);
									ofFill();
									ofDrawRectangle(args.localBounds);
								}
								ofPopStyle();
								break;
							default:
								break;
							}

							// Draw outline for controller sessions
							{
								Nodes::Reworld::Router::Address address;
								{
									address.column = columnIndex;
									address.portal = moduleIndex;
								}
								const auto& selections = parent->getCalibrateControllerSelections();
								if (selections.find(address) != selections.end()) {
									ofPushStyle();
									{
										ofNoFill();
										ofDrawRectangle(args.localBounds);
									}
									ofPopStyle();
								}
							}
							// Draw data
							Models::Reworld::drawAxisAngles(data->axisAngles, args.localBounds);
							};
						panel->setScissorEnabled(false); // optimisation
						columnPanel->add(panel);
					}

					this->columnsPanel->add(columnPanel);
				}
			}

			//----------
			ofxCvGui::ElementPtr
				Capture::getDataDisplay()
			{
				auto element = ofxCvGui::makeElement();

				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				stack->add(make_shared<ofxCvGui::Widgets::LiveValue<glm::vec3>>("Target", [this]() {
					return this->target;
					}));

				// Select column
				{
					auto button = make_shared<ofxCvGui::Widgets::Toggle>("Select"
						, [this]() {
							if (this->parentSelection) {
								return this->parentSelection->isSelected(this);
							}
							else {
								return false;
							}
						}
						, [this](bool value) {
							if (this->parentSelection) {
								if (this->parentSelection->isSelected(this)) {
									this->parentSelection->deselect();
								}
								else {
									this->parentSelection->select(this);
								}
							}
							else {
								ofLogError() << "EditSelection not initialised";
							}
							});
					button->setDrawGlyph(u8"\uf03a");
					stack->add(button);
				}

				return stack;
			}
		}
	}
}
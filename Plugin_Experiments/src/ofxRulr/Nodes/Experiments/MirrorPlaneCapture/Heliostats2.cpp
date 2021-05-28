#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
#pragma mark Heliostats2
				//----------
				Heliostats2::Heliostats2() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Heliostats2::getTypeName() const {
					return "Halo::Heliostats2";
				}

				//----------
				void Heliostats2::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;
					RULR_NODE_UPDATE_LISTENER;

					this->manageParameters(this->parameters);

					// Panel
					{
						this->panel = ofxCvGui::Panels::makeWidgets();
						this->heliostats.populateWidgets(this->panel);
					}

					this->addInput<Dispatcher>();
				}

				//----------
				void Heliostats2::update() {
					if (this->parameters.dispatcher.pushStaleValues) {
						try {
							this->pushStale(false);
						}
						RULR_CATCH_ALL_TO_ERROR;
					}

					// call update on all heliostats
					{
						auto heliostats = this->heliostats.getAllCaptures();
						for (auto heliostat : heliostats) {
							heliostat->update();
						}
					}
				}

				//----------
				void Heliostats2::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addButton("Add", [this]() {
						auto heliostat = make_shared<Heliostat>();
						this->heliostats.add(heliostat);
						});
					inspector->addButton("Import CSV", [this]() {
						try {
							this->importCSV();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Face towards cursor", [this]() {
						try {
							this->faceAllTowardsCursor();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addButton("Face away", [this]() {
						try {
							this->faceAllAway();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addButton("Push stale", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Push stale values");
							this->pushStale(true);
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						})->onDraw.addListener([this](ofxCvGui::DrawArguments& args) {
							// check if any stale
							auto checkStale = [this]() {
								auto heliostats = this->heliostats.getSelection();
								for (auto heliostat : heliostats) {
									if (heliostat->parameters.servo1.getGoalPositionNeedsPush()) {
										return true;
									}
									if (heliostat->parameters.servo2.getGoalPositionNeedsPush()) {
										return true;
									}
								}
								return false;
							};

							ofPushStyle();
							{
								checkStale() ? ofFill() : ofNoFill();
								ofDrawCircle(50, args.localBounds.height / 2.0f, 10);
							}
							ofPopStyle();
						}, this, 100);

					inspector->addButton("Push all", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Push all values");
							this->pushAll();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Pull all", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Pull all values");
							this->pullAll();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

				//----------
				void Heliostats2::serialize(nlohmann::json& json) {
					this->heliostats.serialize(json["heliostats"]);
				}

				//----------
				void Heliostats2::deserialize(const nlohmann::json& json) {
					if (json.contains("heliostats")) {
						this->heliostats.deserialize(json["heliostats"]);
					}
				}

				//----------
				ofxCvGui::PanelPtr Heliostats2::getPanel() {
					return this->panel;
				}

				//----------
				void Heliostats2::drawWorldStage() {
					auto selection = this->heliostats.getSelection();
					for (auto heliostat : selection) {
						heliostat->drawWorld();
					}
				}

				//----------
				std::vector<shared_ptr<Heliostats2::Heliostat>> Heliostats2::getHeliostats() {
					return this->heliostats.getSelection();
				}

				//----------
				void Heliostats2::add(shared_ptr<Heliostat> heliostat) {
					this->heliostats.add(heliostat);
				}

				//----------
				void Heliostats2::removeHeliostat(shared_ptr<Heliostat> heliostat) {
					this->heliostats.remove(heliostat);
				}

				//----------
				void Heliostats2::importCSV() {
					auto result = ofSystemLoadDialog("Load CSV file", false);
					if (!result.bSuccess) {
						return;
					}
					auto fileContents = (string) ofFile(result.filePath, ofFile::ReadOnly).readToBuffer();
					auto fileLines = ofSplitString(fileContents, "\n");

					// Ignore first line (headers)
					for (size_t i = 1; i < fileLines.size(); i++) {
						auto cols = ofSplitString(fileLines[i], ",");
						if (cols.size() < 7) {
							continue;
						}

						shared_ptr<Heliostats2::Heliostat> heliostat;
						
						auto name = cols[0];
						bool isNew = false;

						// check if heliostat already exists
						{
							auto heliostats = this->heliostats.getAllCaptures();
							for (auto existingHeliostat : heliostats) {
								if (existingHeliostat->parameters.name.get() == name) {
									heliostat = existingHeliostat;
								}
							}

							if (!heliostat) {
								heliostat = make_shared<Heliostats2::Heliostat>();
								heliostat->parameters.name.set(cols[0]);
								isNew = true;
							}
						}

						{
							glm::vec3 value{
								ofToFloat(cols[1])
								, ofToFloat(cols[2])
								, ofToFloat(cols[3])
							};
							heliostat->parameters.hamParameters.position.set(value);
						}

						heliostat->parameters.hamParameters.rotationY.set(ofToFloat(cols[4]));

						heliostat->parameters.servo1.ID.set(ofToInt(cols[5]));
						heliostat->parameters.servo2.ID.set(ofToInt(cols[6]));

						if (isNew) {
							this->add(heliostat);
						}
					}
				}

				//----------
				void Heliostats2::faceAllTowardsCursor() {
					auto heliostats = this->heliostats.getSelection();
					auto cursorInWorld = Graph::World::X().getWorldStage()->getCursorWorld();
					
					auto solverSettings = Solvers::HeliostatActionModel::Navigator::defaultSolverSettings();

					for (auto heliostat : heliostats) {
						heliostat->navigateToReflectPointToPoint(cursorInWorld
							, cursorInWorld
							, solverSettings);
					}
				}

				//----------
				void Heliostats2::faceAllAway() {
					auto heliostats = this->heliostats.getSelection();
					for (auto heliostat : heliostats) {
						heliostat->parameters.servo1.angle.set(this->parameters.awayPose.servo1.get());
						heliostat->parameters.servo2.angle.set(this->parameters.awayPose.servo2.get());
					}
				}

				//----------
				void Heliostats2::pushStale(bool waitUntilComplete) {
					this->throwIfMissingAConnection<Dispatcher>();
					auto dispatcher = this->getInput<Dispatcher>();

					Dispatcher::MultiMoveRequest moveRequest;
					moveRequest.waitUntilComplete = waitUntilComplete;
					moveRequest.timeout = this->parameters.dispatcher.timeout.get();

					// Gather movements
					auto heliostats = this->heliostats.getSelection();
					for (auto heliostat : heliostats) {
						if(heliostat->parameters.servo1.getGoalPositionNeedsPush()) {
							Dispatcher::MultiMoveRequest::Movement movement{
								heliostat->parameters.servo1.ID.get()
							, heliostat->parameters.servo1.goalPosition.get()
							};
							moveRequest.movements.push_back(movement);
						}

						if (heliostat->parameters.servo2.getGoalPositionNeedsPush()) {
							Dispatcher::MultiMoveRequest::Movement movement{
								heliostat->parameters.servo2.ID.get()
							, heliostat->parameters.servo2.goalPosition.get()
							};
							moveRequest.movements.push_back(movement);
						}
					}

					if (moveRequest.movements.empty()) {
						return;
					}

					dispatcher->multiMoveRequest(moveRequest);

					// Mark them as pushed after the request completes succesfully
					for (auto heliostat : heliostats) {
						heliostat->parameters.servo1.markGoalPositionPushed();
						heliostat->parameters.servo2.markGoalPositionPushed();
					}
				}

				//----------
				void Heliostats2::pushAll() {
					this->throwIfMissingAConnection<Dispatcher>();
					auto dispatcher = this->getInput<Dispatcher>();
					
					Dispatcher::MultiMoveRequest moveRequest;
					moveRequest.waitUntilComplete = true;

					auto heliostats = this->heliostats.getSelection();
					for (auto heliostat : heliostats) {
						{
							Dispatcher::MultiMoveRequest::Movement movement{
								heliostat->parameters.servo1.ID.get()
							, heliostat->parameters.servo1.goalPosition.get()
							};
							moveRequest.movements.push_back(movement);
						}

						{
							Dispatcher::MultiMoveRequest::Movement movement{
								heliostat->parameters.servo2.ID.get()
							, heliostat->parameters.servo2.goalPosition.get()
							};
							moveRequest.movements.push_back(movement);
						}
					}

					dispatcher->multiMoveRequest(moveRequest);
				}

				//----------
				void Heliostats2::pullAll() {
					this->throwIfMissingAConnection<Dispatcher>();
					auto heliostats = this->heliostats.getSelection();
					auto dispatcher = this->getInput<Dispatcher>();

					// gather servo mappings
					map<Dispatcher::ServoID, ServoParameters*> servos;
					for (auto heliostat : heliostats) {
						servos.emplace(heliostat->parameters.servo1.ID.get()
							, &heliostat->parameters.servo1);
						servos.emplace(heliostat->parameters.servo2.ID.get()
							, &heliostat->parameters.servo2);
					}

					// convert into vector for request
					Dispatcher::MultiGetRequest multiGetRequest;
					for (const auto& it : servos) {
						multiGetRequest.servoIDs.push_back(it.first);
					}

					// get the values
					{
						multiGetRequest.registerName = "Present Position";
						auto response = dispatcher->multiGetRequest(multiGetRequest);
						if (servos.size() != response.size()) {
							throw(ofxRulr::Exception("Size mismatch"));
						}
						for (int i = 0; i < servos.size(); i++) {
							servos[multiGetRequest.servoIDs[i]]->setPresentPosition(response[i]);
						}
					}
				}

				//----------
				cv::Mat Heliostats2::drawMirrorFaceMask(shared_ptr<Heliostat> heliostat, const ofxRay::Camera& cameraView) {
					ofFbo fbo;
					{
						ofFbo::Settings fboSettings;
						{
							fboSettings.width = cameraView.getWidth();
							fboSettings.height = cameraView.getHeight();
							fboSettings.internalformat = GL_RGBA;
							fboSettings.useDepth = true;
							fboSettings.depthStencilInternalFormat = GL_DEPTH_COMPONENT24;
							fboSettings.minFilter = GL_NEAREST;
							fboSettings.maxFilter = GL_NEAREST;
							fboSettings.numSamples = 0;
						}
						fbo.allocate(fboSettings);
					}

					auto heliostats = this->heliostats.getSelection();

					// Draw all the mirror faces into the heliostat
					fbo.begin();
					{
						ofClear(0, 0);

						cameraView.beginAsCamera();
						{
							ofEnableDepthTest();

							ofPushStyle();
							{
								// Draw the selected heliostat mirror as white
								ofSetColor(255);
								heliostat->drawMirrorFace();

								// Draw all other heliostat mirrors as black
								ofSetColor(0);
								for (auto otherHeliostat : heliostats) {
									if (otherHeliostat == heliostat) {
										continue;
									}
									else {
										otherHeliostat->drawMirrorFace();
									}
								}
							}
							ofPopStyle();
							ofDisableDepthTest();
						}
						cameraView.endAsCamera();
					}
					fbo.end();

					// read back the pixels
					ofPixels pixels;
					fbo.readToPixels(pixels);

					// flip the y of the image (fbo's are returned upside-down)
					auto imageRGB = ofxCv::toCv(pixels);
					cv::Mat image;
					cv::extractChannel(imageRGB, image, 0);
					cv::Mat result;
					cv::flip(image, result, 0);

					return result;
				}

#pragma mark Heliostat
				//----------
				Heliostats2::Heliostat::Heliostat() {
					RULR_SERIALIZE_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					// Taken from servos 181, 182 after hand calibration
					this->parameters.servo1.angle.setMin(ofMap(389, 0, 4096, -180, 180));
					this->parameters.servo1.angle.setMax(ofMap(3670, 0, 4096, -180, 180));
					this->parameters.servo2.angle.setMin(ofMap(1016, 0, 4096, -180, 180));
					this->parameters.servo2.angle.setMax(ofMap(3059, 0, 4096, -180, 180));
				}

				//----------
				string Heliostats2::Heliostat::getDisplayString() const {
					return this->parameters.name.get();
				}

				//----------
				void Heliostats2::Heliostat::drawWorld() {
					ofxCvGui::Utils::drawTextAnnotation(this->parameters.name
						, this->parameters.hamParameters.position
						, this->color);

					auto isBeingInspected = this->isBeingInspected();
					auto color = isBeingInspected
						? ofColor(255, 255, 255)
						: ofColor(100, 100, 100);

					ofPushStyle();
					{
						ofSetColor(color);
						ofNoFill();

						ofPushMatrix();
						{
							ofTranslate(this->parameters.hamParameters.position);
							// Base
							ofDrawBox({ 0, 0.21, 0 }, 0.22, 0.05, 0.13);

							// Axis
							if (isBeingInspected) {
								ofDrawAxis(0.1f);
							}

							// Axis 1
							{
								ofMultMatrix(glm::rotate<float>(this->parameters.servo1.angle.get() * DEG_TO_RAD
									, this->parameters.hamParameters.axis1.rotationAxis.get()));

								// Body
								ofDrawBox({ 0, 0.07, 0 }, 0.22, 0.23, 0.13);

								// Axis 1
								{
									ofMultMatrix(glm::rotate<float>(this->parameters.servo2.angle.get() * DEG_TO_RAD
										, this->parameters.hamParameters.axis2.rotationAxis.get()));

									ofMultMatrix(glm::translate(glm::vec3( 0.0f, -this->parameters.hamParameters.mirrorOffset.get(), 0.0f )));
									ofRotateDeg(-90, 1.0f, 0.0f, 0.0f);
									ofDrawCircle(0.0f, 0.0f, 0.35f / 2.0f);
								}
							}
						}
						ofPopMatrix();
					}
					ofPopStyle();

					// Mirror face
					if (isBeingInspected) {
						glm::vec3 mirrorCenter, mirrorNormal;
						Solvers::HeliostatActionModel::getMirrorCenterAndNormal({
							this->parameters.servo1.angle.get()
							, this->parameters.servo2.angle.get()
							}
							, this->getHeliostatActionModelParameters()
							, mirrorCenter
							, mirrorNormal);

						ofDrawArrow(mirrorCenter, mirrorCenter + 0.2f * mirrorNormal, 0.01f);

						ofPushStyle();
						{
							ofSetColor(150);
							this->drawMirrorFace();
						}
						ofPopStyle();
					}
				}

				//----------
				void Heliostats2::Heliostat::drawMirrorFace() {
					glm::vec3 mirrorCenter, mirrorNormal;
					Solvers::HeliostatActionModel::getMirrorCenterAndNormal({
						this->parameters.servo1.angle.get()
						, this->parameters.servo2.angle.get()
						}
						, this->getHeliostatActionModelParameters()
						, mirrorCenter
						, mirrorNormal);

					Solvers::HeliostatActionModel::drawMirror(mirrorCenter
						, mirrorNormal
						, this->parameters.diameter.get());
				}

				//----------
				void Heliostats2::Heliostat::update() {
					this->parameters.servo1.update();
					this->parameters.servo2.update();
				}

				//----------
				string Heliostats2::Heliostat::getName() const {
					return this->parameters.name.get();
				}

				//----------
				void Heliostats2::Heliostat::serialize(nlohmann::json& json) {
					Utils::serialize(json, "parameters", this->parameters);
				}

				//----------
				void Heliostats2::Heliostat::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "parameters", this->parameters);
				}

				//----------
				void Heliostats2::Heliostat::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
					auto inspector = inspectArgs.inspector;
					inspector->addParameterGroup(this->parameters);
					inspector->addButton("Flip 180", [this]() {
						try {
							this->flip();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
				}

				//----------
				Solvers::HeliostatActionModel::Parameters<float> Heliostats2::Heliostat::getHeliostatActionModelParameters() const {
					Solvers::HeliostatActionModel::Parameters<float> parameters;
					parameters.position = this->parameters.hamParameters.position.get();

					parameters.axis1.polynomial = this->parameters.hamParameters.axis1.polynomial.get();
					parameters.axis1.rotationAxis = this->parameters.hamParameters.axis1.rotationAxis.get();
					parameters.axis1.angleRange.minimum = this->parameters.servo1.angle.getMin();
					parameters.axis1.angleRange.maximum = this->parameters.servo1.angle.getMax();

					parameters.axis2.polynomial = this->parameters.hamParameters.axis2.polynomial.get();
					parameters.axis2.rotationAxis = this->parameters.hamParameters.axis2.rotationAxis.get();
					parameters.axis2.angleRange.minimum = this->parameters.servo2.angle.getMin();
					parameters.axis2.angleRange.maximum = this->parameters.servo2.angle.getMax();

					parameters.mirrorOffset = this->parameters.hamParameters.mirrorOffset.get();
					return parameters;
				}

				//----------
				void Heliostats2::Heliostat::setHeliostatActionModelParameters(const Solvers::HeliostatActionModel::Parameters<float> & parameters) {
					this->parameters.hamParameters.position.set(parameters.position);

					this->parameters.hamParameters.axis1.polynomial.set(parameters.axis1.polynomial);
					this->parameters.hamParameters.axis1.rotationAxis.set(parameters.axis1.rotationAxis);

					this->parameters.hamParameters.axis2.polynomial.set(parameters.axis2.polynomial);
					this->parameters.hamParameters.axis2.rotationAxis.set(parameters.axis2.rotationAxis);

					this->parameters.hamParameters.mirrorOffset.set(parameters.mirrorOffset);
				}

				//----------
				void Heliostats2::Heliostat::flip() {
					Solvers::HeliostatActionModel::AxisAngles<float> axisAngles;
					axisAngles.axis1 = this->parameters.servo1.angle.get();
					axisAngles.axis2 = this->parameters.servo2.angle.get();

					axisAngles.axis1 += axisAngles.axis1 > 0.0f
						? -180.0f
						: 180.0f;
					axisAngles.axis2 *= -1.0f;

					if (Solvers::HeliostatActionModel::Navigator::validate(this->getHeliostatActionModelParameters()
						, axisAngles)) {
						this->parameters.servo1.angle.set(axisAngles.axis1);
						this->parameters.servo2.angle.set(axisAngles.axis2);
					}
					else {
						throw(ofxRulr::Exception("Cannot flip axis"));
					}
				}

				//----------
				void Heliostats2::Heliostat::navigateToNormal(const glm::vec3& normal, const ofxCeres::SolverSettings& solverSettings) {
					Solvers::HeliostatActionModel::AxisAngles<float> priorAngles{
						this->parameters.servo1.angle
						, this->parameters.servo2.angle
					};
					auto hamParameters = this->getHeliostatActionModelParameters();

					auto result = Solvers::HeliostatActionModel::Navigator::solveConstrained(hamParameters
						, [&](const Solvers::HeliostatActionModel::AxisAngles<float>& initialAngles) {
							return Solvers::HeliostatActionModel::Navigator::solveNormal(hamParameters
								, normal
								, initialAngles
								, solverSettings);
						}, priorAngles);

					this->parameters.servo1.angle = result.solution.axisAngles.axis1;
					this->parameters.servo2.angle = result.solution.axisAngles.axis2;

					if (!result.isConverged()) {
						throw(ofxRulr::Exception("Couldn't navigate heliostat to normal : " + result.errorMessage));
					}
				}

				//----------
				void Heliostats2::Heliostat::navigateToReflectPointToPoint(const glm::vec3& pointA, const glm::vec3& pointB, const ofxCeres::SolverSettings& solverSettings) {
					Solvers::HeliostatActionModel::AxisAngles<float> priorAngles{
							this->parameters.servo1.angle
							, this->parameters.servo2.angle
					};
					auto hamParameters = this->getHeliostatActionModelParameters();

					auto result = Solvers::HeliostatActionModel::Navigator::solveConstrained(hamParameters
						, [&](const Solvers::HeliostatActionModel::AxisAngles<float>& initialAngles) {
							return Solvers::HeliostatActionModel::Navigator::solvePointToPoint(hamParameters
								, pointA
								, pointB
								, initialAngles
								, solverSettings);
						}, priorAngles);

					this->parameters.servo1.angle = result.solution.axisAngles.axis1;
					this->parameters.servo2.angle = result.solution.axisAngles.axis2;

					if (!result.isConverged()) {
						throw(ofxRulr::Exception("Couldn't navigate heliostat point to point: " + result.errorMessage));
					}
				}

				//----------
				ofxCvGui::ElementPtr Heliostats2::Heliostat::getDataDisplay() {
					auto element = ofxCvGui::makeElement();

					vector<ofxCvGui::ElementPtr> widgets;

					widgets.push_back(make_shared<ofxCvGui::Widgets::EditableValue<string>>(this->parameters.name));
					widgets.push_back(make_shared<ofxCvGui::Widgets::Toggle>("Edit"
						, [this]() {
							return this->isBeingInspected();
						}
						, [this](bool value) {
							if (value) {
								ofxCvGui::InspectController::X().inspect(this->shared_from_this());
							}
							else {
								ofxCvGui::InspectController::X().clear();
							}
						}));


					for (auto& widget : widgets) {
						element->addChild(widget);
					}

					element->onBoundsChange += [this, widgets](ofxCvGui::BoundsChangeArguments& args) {
						auto bounds = args.localBounds;
						bounds.height = 40.0f;

						for (auto& widget : widgets) {
							widget->setBounds(bounds);
							bounds.y += bounds.height;
						}
					};

					element->setHeight(widgets.size() * 40 + 10);

					return element;
				}

#pragma mark Heliostats2::ServoParameters
				//----------
				Heliostats2::ServoParameters::ServoParameters(const string& name, HAMParameters::AxisParameters& axisParameters)
				: axisParameters(axisParameters)
				{
					this->setName(name);
					this->add(this->ID);
					this->add(this->angle);
					this->add(this->goalPosition);
				}

				//----------
				void Heliostats2::ServoParameters::update() {
					if (this->cachedAngle != this->angle.get()) {
						this->calculateGoalPosition();
						this->cachedAngle = this->angle.get();
					}
					if (this->cachedGoalPosition != this->goalPosition.get()) {
						this->goalPositionNeedsPush = true;
					}
				}

				//----------
				bool Heliostats2::ServoParameters::getGoalPositionNeedsPush() const {
					return this->goalPositionNeedsPush;
				}

				//----------
				void Heliostats2::ServoParameters::markGoalPositionPushed() {
					this->goalPositionNeedsPush = false;
					this->cachedGoalPosition = this->goalPosition.get();
				}

				//----------
				void Heliostats2::ServoParameters::calculateGoalPosition() {
					auto angle = this->angle;
					const auto& polynomial = this->axisParameters.polynomial.get();

					auto result = Solvers::HeliostatActionModel::SolvePosition::solvePosition(angle
						, polynomial
						, this->goalPosition.get());
					if (!result.isConverged()) {
						throw(ofxRulr::Exception("Failed to invert polynomial"));
					}

					auto goalPosition = (int)result.solution;

					goalPosition = goalPosition > 4095
						? 4095
						: goalPosition < 0
							? 0
							: goalPosition;
					this->goalPosition.set(goalPosition);
				}

				//----------
				void Heliostats2::ServoParameters::setPresentPosition(const Dispatcher::RegisterValue& registerValue) {
					auto result = Solvers::HeliostatActionModel::positionToAngle<float>((float) registerValue
						, this->axisParameters.polynomial.get());
				}
			}
		}
	}
}
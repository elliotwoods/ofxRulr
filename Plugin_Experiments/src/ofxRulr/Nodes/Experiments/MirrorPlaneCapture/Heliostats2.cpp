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
				Heliostats2::~Heliostats2() {
					if (this->dispatcherThread.isRunning) {
						this->dispatcherThread.isClosing = true;
						this->dispatcherThread.actionsCV.notify_one();
						this->dispatcherThread.thread.join();
					}
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
						this->panel->addButton("Select range...", [this]() {
							auto result = ofSystemTextBoxDialog("Select range, e.g. '3-4, 10'");
							if (!result.empty()) {
								this->selectRangeByString(result);
							}
							});
					}

					this->addInput<Dispatcher>();

					this->dispatcherThread.thread = std::thread([this]() {
						while (!this->dispatcherThread.isClosing) {
							std::unique_lock<std::mutex> lock(this->dispatcherThread.actionsLock);
							this->dispatcherThread.actionsCV.wait_for(lock, std::chrono::seconds(1));
							if (!this->dispatcherThread.isClosing) {
								if (!this->dispatcherThread.actions.empty()) {
									auto action = this->dispatcherThread.actions.front();
									this->dispatcherThread.actions.pop_front();
									lock.unlock();

									try {
										action->action();
										action->promise.set_value();
									}
									RULR_CATCH_ALL_TO_ERROR;
								}
							}
						}
						});
					this->dispatcherThread.isRunning = true;
				}

				//----------
				void Heliostats2::update() {
					if (this->parameters.dispatcher.pushStaleValues) {
						// Check if we already have a pushStale in the queue
						bool havePushStale = false;
						this->dispatcherThread.actionsLock.lock();
						{
							for (const auto& action : this->dispatcherThread.actions) {
								if (action->containsPushStale) {
									havePushStale = true;
									break;
								}
							}
						}
						this->dispatcherThread.actionsLock.unlock();

						if (!havePushStale) {
							try {
								this->pushStale(false, false);
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
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

					inspector->addSpacer();

					inspector->addButton("Push stale", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Push stale values");
							this->pushStale(true, true);
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
							this->pushAll(false);
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Pull all positions", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Pull all positions");
							this->pullAllPositions();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Pull all limits", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Pull all limits");
							this->pullAllLimits();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Center poly on limits", [this]() {
						try {
							Utils::ScopedProcess scopedProcess("Pull limits to poly");
							this->centerPolyOnLimits();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addSpacer();

					inspector->addButton("Reset detail parameters", [this]() {
						try {
							this->resetDetailParameters();
						}
						RULR_CATCH_ALL_TO_ALERT;
						});

					inspector->addSpacer();

					inspector->addButton("Torque on", [this]() {
						try {
							this->setTorqueEnable(true);
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Torque off", [this]() {
						try {
							this->setTorqueEnable(false);
						}
						RULR_CATCH_ALL_TO_ALERT;
						});
					inspector->addButton("Face away", [this]() {
						try {
							this->faceAllAway();
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
					Heliostat::DrawParameters drawParameters;
					{
						drawParameters.nodeIsSelected = this->isBeingInspected();

						// should draw labels
						drawParameters.labels = isActive(this, this->parameters.draw.labels);

						drawParameters.servoIndices = this->parameters.draw.servoIndices.get();
					}

					for (auto heliostat : selection) {
						heliostat->drawWorld(drawParameters);
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
				// https://paper.dropbox.com/doc/KC81-Heliostat-CSV-format--CECtlPII1sunrWKrts2rUb6~Ag-a7eEMn7EiS2rfMG324gBY
				void Heliostats2::importCSV() {
					auto result = ofSystemLoadDialog("Load CSV file", false);
					if (!result.bSuccess) {
						return;
					}
					auto fileContents = (string) ofFile(result.filePath, ofFile::ReadOnly).readToBuffer();
					auto fileLines = ofSplitString(fileContents, "\n");

					// Ignore first line (headers)
					for (size_t i = 1; i < fileLines.size(); i++) {
						bool rightTangent = i % 2 == 1;

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

						if (cols.size() >= 12) {
							auto axis2AngleOffset = ofToFloat(cols[11]);
							heliostat->parameters.servo2.angleOffset.set(axis2AngleOffset);
						}
						heliostat->parameters.rightTangent.set(rightTangent);

						if (isNew) {
							this->add(heliostat);
						}
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
				void Heliostats2::setTorqueEnable(bool value) {
					this->throwIfMissingAConnection<Dispatcher>();
					auto dispatcher = this->getInput<Dispatcher>();

					Dispatcher::MultiSetRequest multiSetRequest;
					auto heliostats = this->getHeliostats();
					multiSetRequest.registerName = "Torque Enable";
					for (auto heliostat : heliostats) {
						multiSetRequest.servoValues[heliostat->parameters.servo1.ID.get()] = value ? 1 : 0;
						multiSetRequest.servoValues[heliostat->parameters.servo2.ID.get()] = value ? 1 : 0;
					}
					if (heliostats.empty()) {
						return;
					}

					dispatcher->multiSetRequest(multiSetRequest);
				}

				//----------
				void Heliostats2::pushStale(bool blocking, bool waitUntilComplete) {
					this->throwIfMissingAConnection<Dispatcher>();
					auto dispatcher = this->getInput<Dispatcher>();

					// Update all heliostats (e.g. in case calculations need to be made)
					auto heliostats = this->heliostats.getSelection();
					for (auto heliostat : heliostats) {
						heliostat->update();
					}

					Dispatcher::MultiMoveRequest moveRequest;
					moveRequest.waitUntilComplete = waitUntilComplete;
					moveRequest.timeout = this->parameters.dispatcher.timeout.get();

					// Gather movements
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

					auto perform = [=]() {
						dispatcher->multiMoveRequest(moveRequest);

						// Mark them as pushed after the request completes succesfully
						for (auto heliostat : heliostats) {
							heliostat->parameters.servo1.markGoalPositionPushed();
							heliostat->parameters.servo2.markGoalPositionPushed();
						}
					};

					if (blocking) {
						perform();
					}
					else {
						this->dispatcherThread.actionsLock.lock();
						{
							auto action = make_shared<DispatcherThread::Action>(perform, true);
							this->dispatcherThread.actions.push_back(action);
						}
						this->dispatcherThread.actionsLock.unlock();

						this->dispatcherThread.actionsCV.notify_all();
					}
				}

				//----------
				void Heliostats2::pushAll(bool blocking) {
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

					auto perform = [=]() {
						dispatcher->multiMoveRequest(moveRequest);
					};

					if (blocking) {
						perform();
					}
					else {
						this->dispatcherThread.actionsLock.lock();
						{
							auto action = make_shared<DispatcherThread::Action>(perform, true);
							this->dispatcherThread.actions.push_back(action);
						}
						this->dispatcherThread.actionsLock.unlock();

						this->dispatcherThread.actionsCV.notify_all();
					}
				}

				//----------
				void Heliostats2::pullAllPositions() {
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
				void Heliostats2::pullAllLimits() {
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

					// get the maximum
					{
						multiGetRequest.registerName = "Max Position Limit";
						auto response = dispatcher->multiGetRequest(multiGetRequest);
						if (servos.size() != response.size()) {
							throw(ofxRulr::Exception("Size mismatch"));
						}
						for (int i = 0; i < servos.size(); i++) {
							servos[multiGetRequest.servoIDs[i]]->setMaxPosition(response[i]);
						}
					}

					// get the maximum
					{
						multiGetRequest.registerName = "Min Position Limit";
						auto response = dispatcher->multiGetRequest(multiGetRequest);
						if (servos.size() != response.size()) {
							throw(ofxRulr::Exception("Size mismatch"));
						}
						for (int i = 0; i < servos.size(); i++) {
							servos[multiGetRequest.servoIDs[i]]->setMinPosition(response[i]);
						}
					}
				}

				//----------
				void Heliostats2::resetDetailParameters() {
					auto heliostats = this->heliostats.getSelection();
					for (auto heliostat : heliostats) {
						heliostat->parameters.hamParameters.axis1.rotationAxis.set({ 0, -1, 0 });
						heliostat->parameters.hamParameters.axis2.rotationAxis.set({ 1, 0, 0 });
						heliostat->parameters.hamParameters.axis1.polynomial.set({ 0, 1, 0 });
						heliostat->parameters.hamParameters.axis2.polynomial.set({ 0, 1, 0 });
					}
				}

				//----------
				void Heliostats2::centerPolyOnLimits() {
					auto heliostats = this->getHeliostats();
					for (auto heliostat : heliostats) {
						heliostat->parameters.servo1.centerPolyOnLimits();
						heliostat->parameters.servo2.centerPolyOnLimits();
					}
				}

				//----------
				void Heliostats2::selectRangeByString(const string& text) {
					this->heliostats.selectNone();
					auto allHeliostats = this->heliostats.getAllCaptures();

					auto terms = ofSplitString(text, ",");
					for (const auto& term : terms) {
						auto splitAsRange = ofSplitString(term, "-");
						if (splitAsRange.size() >= 2) {
							// range
							auto startRange = ofToInt(splitAsRange[0]);
							auto endRange = ofToInt(splitAsRange[1]);
							if (endRange > startRange) {
								for (auto heliostat : allHeliostats) {
									auto heliostatIndex = ofToInt(heliostat->parameters.name.get());
									if (heliostatIndex >= startRange
										&& heliostatIndex <= endRange) {
										heliostat->setSelected(true);
									}
								}
							}
						}
						else {
							auto index = ofToInt(term);
							for (auto heliostat : allHeliostats) {
								auto heliostatIndex = ofToInt(heliostat->parameters.name.get());
								if (heliostatIndex == index) {
									heliostat->setSelected(true);
								}
							}
						}
					}
				}

				//----------
				cv::Mat Heliostats2::drawMirrorFaceMask(shared_ptr<Heliostat> heliostat, const ofxRay::Camera& cameraView, float mirrorScale) {
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
								heliostat->drawMirrorFace(mirrorScale);

								// Draw all other heliostat mirrors as black
								ofSetColor(0);
								for (auto otherHeliostat : heliostats) {
									if (otherHeliostat == heliostat) {
										continue;
									}
									else {
										otherHeliostat->drawMirrorFace(mirrorScale);
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

				//----------
				void Heliostats2::Heliostat::clampAxisValuesToLimits() {
					this->parameters.servo1.clampAngleToLimits();
					this->parameters.servo2.clampAngleToLimits();
				}

#pragma mark Heliostat
				//----------
				Heliostats2::Heliostat::Heliostat() {
					RULR_SERIALIZE_LISTENERS;
					RULR_NODE_INSPECTOR_LISTENER;

					// Taken from servos 181, 182 after hand calibration
					this->parameters.servo1.angle.setMin(-140);
					this->parameters.servo1.angle.setMax(140);
					this->parameters.servo2.angle.setMin(-90);
					this->parameters.servo2.angle.setMax(90);
				}

				//----------
				string Heliostats2::Heliostat::getDisplayString() const {
					return this->parameters.name.get();
				}

				//----------
				void Heliostats2::Heliostat::drawWorld(const DrawParameters& drawParameters) {
					auto isBeingInspected = this->isBeingInspected();
					auto color = isBeingInspected
						? ofColor(255, 255, 255)
						: ofColor(100, 100, 100);

					if(drawParameters.labels) {
						stringstream ss;
						ss << this->parameters.name;
						if (drawParameters.servoIndices) {
							ss << " : " << this->parameters.servo1.ID.get() << "," << this->parameters.servo2.ID.get();
						}
						ofxCvGui::Utils::drawTextAnnotation(ss.str()
							, this->parameters.hamParameters.position
							, this->color);
					}

					ofPushStyle();
					{
						ofSetColor(color);
						ofNoFill();

						ofPushMatrix();
						{
							ofTranslate(this->parameters.hamParameters.position.get());

							ofRotateDeg(this->parameters.hamParameters.rotationY.get(), 0, 1, 0);

							// Base
							ofDrawBox({ 0, 0.21, 0 }, 0.22, 0.05, 0.13);

							// Arrow
							ofDrawLine({ -0.11, 0.21f + 0.025f, 0 }, { 0.0f, 0.21f + 0.025f, -0.13f / 2.0f });
							ofDrawLine({ +0.11, 0.21f + 0.025f, 0 }, { 0.0f, 0.21f + 0.025f, -0.13f / 2.0f });

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
						auto mirrorPlane = Solvers::HeliostatActionModel::getMirrorPlane({
							this->parameters.servo1.angle.get()
							, this->parameters.servo2.angle.get()
							}
							, this->getHeliostatActionModelParameters());

						ofDrawArrow(mirrorPlane.center, mirrorPlane.center + 0.2f * mirrorPlane.normal, 0.01f);

						ofPushStyle();
						{
							ofSetColor(150);
							this->drawMirrorFace(1.0f);
						}
						ofPopStyle();
					}
				}

				//----------
				void Heliostats2::Heliostat::drawMirrorFace(float mirrorScale) {
					auto mirrorPlane = Solvers::HeliostatActionModel::getMirrorPlane({
						this->parameters.servo1.angle.get()
						, this->parameters.servo2.angle.get()
						}
						, this->getHeliostatActionModelParameters());

					Solvers::HeliostatActionModel::drawMirror(mirrorPlane.center
						, mirrorPlane.normal
						, this->parameters.diameter.get() * mirrorScale);
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

					{
						auto& jsonLimits = json["limits"];
						Utils::serialize(jsonLimits, "servo1MinAngle", this->parameters.servo1.angle.getMin());
						Utils::serialize(jsonLimits, "servo1MaxAngle", this->parameters.servo1.angle.getMax());
						Utils::serialize(jsonLimits, "servo2MinAngle", this->parameters.servo2.angle.getMin());
						Utils::serialize(jsonLimits, "servo2MaxAngle", this->parameters.servo2.angle.getMax());
					}
				}

				//----------
				void Heliostats2::Heliostat::deserialize(const nlohmann::json& json) {
					Utils::deserialize(json, "parameters", this->parameters);

					if (json.contains("limits")) {
						const auto& jsonLimits = json["limits"];
						float value;
						if (Utils::deserialize(jsonLimits, "servo1MinAngle", value)) {
							this->parameters.servo1.angle.setMin(value);
						}
						if (Utils::deserialize(jsonLimits, "servo1MaxAngle", value)) {
							this->parameters.servo1.angle.setMax(value);
						}
						if (Utils::deserialize(jsonLimits, "servo2MinAngle", value)) {
							this->parameters.servo2.angle.setMin(value);
						}
						if (Utils::deserialize(jsonLimits, "servo2MaxAngle", value)) {
							this->parameters.servo2.angle.setMax(value);
						}
					}
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
					parameters.rotationY = this->parameters.hamParameters.rotationY.get();

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

					this->parameters.hamParameters.rotationY.set(parameters.rotationY);

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

					this->update();
				}

				//----------
				void Heliostats2::Heliostat::navigateToNormal(const glm::vec3& normal, const ofxCeres::SolverSettings& solverSettings, bool throwIfOutsideRange) {
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
						}, priorAngles
						, throwIfOutsideRange);

					this->parameters.servo1.angle = result.solution.axisAngles.axis1;
					this->parameters.servo2.angle = result.solution.axisAngles.axis2;

					this->update();
				}

				//----------
				void Heliostats2::Heliostat::navigateToReflectPointToPoint(const glm::vec3& pointA, const glm::vec3& pointB, const ofxCeres::SolverSettings& solverSettings, bool throwIfOutsideRange) {
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
						}, priorAngles
						, throwIfOutsideRange);

					this->parameters.servo1.angle = result.solution.axisAngles.axis1;
					this->parameters.servo2.angle = result.solution.axisAngles.axis2;

					this->update();
				}

				//----------
				void Heliostats2::Heliostat::navigateToReflectVectorToPoint(const glm::vec3& incidentVector
					, const glm::vec3& point
					, const ofxCeres::SolverSettings& solverSettings
					, bool throwIfOutsideRange) {
					Solvers::HeliostatActionModel::AxisAngles<float> priorAngles{
							this->parameters.servo1.angle
							, this->parameters.servo2.angle
					};
					auto hamParameters = this->getHeliostatActionModelParameters();

					auto result = Solvers::HeliostatActionModel::Navigator::solveConstrained(hamParameters
						, [&](const Solvers::HeliostatActionModel::AxisAngles<float>& initialAngles) {
							return Solvers::HeliostatActionModel::Navigator::solveVectorToPoint(hamParameters
								, incidentVector
								, point
								, initialAngles
								, solverSettings);
						}, priorAngles
						, throwIfOutsideRange);

					this->parameters.servo1.angle = result.solution.axisAngles.axis1;
					this->parameters.servo2.angle = result.solution.axisAngles.axis2;

					this->update();
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
					this->add(this->angleOffset);
					this->add(this->goalPosition);
				}

				//----------
				void Heliostats2::ServoParameters::update() {
					this->clampAngleToLimits();
					if (this->cachedAngle != this->angle.get()
						|| this->cachedAngleOffset != this->angleOffset.get()) {
						this->calculateGoalPosition();
						this->cachedAngle = this->angle.get();
						this->cachedAngleOffset = this->angleOffset.get();
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
					auto goalPosition = angleToGoalPosition(this->angle.get());
					this->goalPosition.set(goalPosition);
				}

				//----------
				int Heliostats2::ServoParameters::angleToGoalPosition(float angle) {
					const auto& polynomial = this->axisParameters.polynomial.get();
					const auto& angleOffset = this->angleOffset.get();
					auto result = Solvers::HeliostatActionModel::SolvePosition::solvePosition(angle
						, angleOffset
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
					return goalPosition;
				}

				//----------
				void Heliostats2::ServoParameters::setPresentPosition(const Dispatcher::RegisterValue& registerValue) {
					auto result = Solvers::HeliostatActionModel::positionToAngle<float>((float) registerValue
						, this->axisParameters.polynomial.get()
						, this->angleOffset.get());
				}

				//----------
				void Heliostats2::ServoParameters::setMinPosition(const Dispatcher::RegisterValue& registerValue) {
					auto result = Solvers::HeliostatActionModel::positionToAngle<float>((float)registerValue
						, this->axisParameters.polynomial.get()
						, this->angleOffset.get());
					this->angle.setMin(result);
				}

				//----------
				void Heliostats2::ServoParameters::setMaxPosition(const Dispatcher::RegisterValue& registerValue) {
					auto result = Solvers::HeliostatActionModel::positionToAngle<float>((float)registerValue
						, this->axisParameters.polynomial.get()
						, this->angleOffset.get());
					this->angle.setMax(result);
				}

				//----------
				void Heliostats2::ServoParameters::clampAngleToLimits() {
					if (this->angle.get() < this->angle.getMin()) {
						this->angle.set(this->angle.getMin());
					}
					if (this->angle.get() > this->angle.getMax()) {
						this->angle.set(this->angle.getMax());
					}
				}

				//----------
				void Heliostats2::ServoParameters::centerPolyOnLimits() {
					auto centerAngle = (this->angle.getMin() + this->angle.getMax()) / 2.0f;
					auto centerPosition = this->angleToGoalPosition(centerAngle);
					auto centerOffset = centerPosition - 2048;
					auto poly = this->axisParameters.polynomial.get();
					poly.x = -centerOffset;
					this->axisParameters.polynomial.set(poly);
				}
			}
		}
	}
}
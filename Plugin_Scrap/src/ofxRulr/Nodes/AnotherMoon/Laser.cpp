#include "pch_Plugin_Scrap.h"
#include "Laser.h"
#include "Lasers.h"

#include "ofxRulr/Solvers/NavigateToWorldPoint.h"

using namespace ofxRulr::Data::AnotherMoon;

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			Laser::Laser()
			{
				RULR_SERIALIZE_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				this->rigidBody->init();
				this->parameters.intrinsics.fov.addListener(this, &Laser::callbackIntrinsicsChange);
				this->parameters.intrinsics.fov2.addListener(this, &Laser::callbackIntrinsicsChange);
			}

			//----------
			Laser::~Laser()
			{
				this->shutdown();
			}

			//----------
			void
				Laser::setParent(Lasers* parent)
			{
				this->parent = parent;
			}

			//----------
			string
				Laser::getDisplayString() const
			{
				stringstream ss;
				ss << this->getLabelName() << " (" << this->rigidBody->getPosition() << ")";
				return ss.str();
			}

			//----------
			void
				Laser::update()
			{
				this->rigidBody->setName(this->getLabelName());
				this->rigidBody->setColor(this->color);
				this->rigidBody->update();

				// Keep alive message
				{
					// Selected rather than inspected
					bool shouldSend;
					switch(this->parameters.communications.keepAlive.enabled.get().get()) {
					case WhenActive::Always:
						shouldSend = true;
						break;
					case WhenActive::Selected:
						shouldSend = this->isSelected();
						break;
					case WhenActive::Never:
					default:
						shouldSend = false;
						break;
					}
					if (shouldSend) {
						auto now = chrono::system_clock::now();
						auto period = chrono::milliseconds(this->parameters.communications.keepAlive.period.get());
						if (this->lastKeepAliveSent + period < now) {
							this->lastKeepAliveSent = now;
							auto message = this->createOutgoingMessageOnce();
							message->setAddress("/keepAlive");
							this->sendMessage(message);
						}
					}
				}

				// Check all parameters for stale and push if needed
				{
					if(this->parameters.deviceState.state.get() != this->sentDeviceParameters.state.get()) {
						this->pushState();
					}

					if (this->parameters.deviceState.localKeepAlive.get() != this->sentDeviceParameters.localKeepAlive.get()) {
						this->pushLocalKeepAlive();
					}

					if (this->parameters.deviceState.projection.source.get() != this->sentDeviceParameters.projection.source.get()) {
						this->pushSource();
					}

					if (this->parameters.deviceState.projection.color.red.get() != this->sentDeviceParameters.projection.color.red.get()
						|| this->parameters.deviceState.projection.color.green.get() != this->sentDeviceParameters.projection.color.green.get()
						|| this->parameters.deviceState.projection.color.blue.get() != this->sentDeviceParameters.projection.color.blue.get()) {
						this->pushColor();
					}

					if (this->parameters.deviceState.projection.transform.sizeX.get() != this->sentDeviceParameters.projection.transform.sizeX.get()
						|| this->parameters.deviceState.projection.transform.sizeY.get() != this->sentDeviceParameters.projection.transform.sizeY.get()
						|| this->parameters.deviceState.projection.transform.offsetX.get() != this->sentDeviceParameters.projection.transform.offsetX.get()
						|| this->parameters.deviceState.projection.transform.offsetY.get() != this->sentDeviceParameters.projection.transform.offsetY.get()) {
						this->pushTransform();
					}

					if (this->parameters.deviceState.projection.circle.sizeX.get() != this->sentDeviceParameters.projection.circle.sizeX.get()
						|| this->parameters.deviceState.projection.circle.sizeY.get() != this->sentDeviceParameters.projection.circle.sizeY.get()
						|| this->parameters.deviceState.projection.circle.offsetX.get() != this->sentDeviceParameters.projection.circle.offsetX.get()
						|| this->parameters.deviceState.projection.circle.offsetY.get() != this->sentDeviceParameters.projection.circle.offsetY.get()
						|| this->parameters.deviceState.projection.circle.frequency.get() != this->sentDeviceParameters.projection.circle.frequency.get()
						|| this->parameters.deviceState.projection.circle.phase.get() != this->sentDeviceParameters.projection.circle.phase.get()) {
						this->pushCircle();
					}
				}

				// Heartbeats
				{
					this->isFrameNewAck.update();
					this->isFrameNewIncoming.update();
					this->isFrameNewTransmit.update();
				}
			}

			//----------
			void
				Laser::serialize(nlohmann::json& json)
			{
				Utils::serialize(json, this->parameters);

				this->rigidBody->serialize(json["rigidBody"]);
			}

			//----------
			void
				Laser::deserialize(const nlohmann::json& json)
			{
				Utils::deserialize(json, this->parameters);

				{
					if (json.contains("rigidBody")) {
						this->rigidBody->deserialize(json["rigidBody"]);
					}
				}

				// Legacy
				{
					if (json.contains("Laser")) {
						const auto& jsonLaser = json["Laser"];

						// Before we changed settings to communications and intrinsics
						if (jsonLaser.contains("Settings")) {
							const auto& jsonSettings = jsonLaser["Settings"];
							if (jsonSettings.contains("Address")) {
								this->parameters.serialNumber.set(jsonSettings["Address"].get<int>());
							}
							if (jsonSettings.contains("FOV")) {
								const auto& jsonFOV = jsonSettings["FOV"];
								if (jsonFOV.size() >= 2) {
									glm::vec2 fov{
										jsonFOV[0].get<float>()
										, jsonFOV[1].get<float>()
									};
									this->parameters.intrinsics.fov.set(fov);
								}
							}
						}

						// Before we had Position and Address seperate
						if (jsonLaser.contains("Communications")) {
							if (jsonLaser["Communications"].contains("Address")) {
								if (!jsonLaser.contains("Position index")) {
									this->parameters.positionIndex.set(jsonLaser["Communications"]["Address"].get<int>());
								}
							}
						}
					}
				}
			}

			//----------
			void
				Laser::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;
				{
					inspector->addTitle("Last picture preview", ofxCvGui::Widgets::Title::Level::H3);
					auto element = ofxCvGui::makeElement();
					element->setHeight(inspector->getWidth());
					inspector->add(element);

					element->onDraw += [this](ofxCvGui::DrawArguments& args) {
						ofPushStyle();
						{
							ofSetColor(200);
							ofNoFill();
							ofDrawRectangle(args.localBounds);
						}
						ofPopStyle();

						if (!this->lastPictureSent.empty()) {
							ofPolyline line;

							for (const auto& projectionPoint : this->lastPictureSent) {
								line.addVertex(ofMap(projectionPoint.x, -1, 1, 0, args.localBounds.width)
									, ofMap(projectionPoint.y, 1, -1, 0, args.localBounds.height)
									, 0);
							}
							line.close();
							line.draw();
						}
					};
				}

				inspector->addParameterGroup(this->parameters);
			}

			//----------
			void
				Laser::drawWorldStage(const DrawArguments& args)
			{
				// Draw Rigid body
				if (args.rigidBody) {
					this->rigidBody->drawWorldStage();
				}

				// In object space
				{
					ofPushMatrix();
					{
						ofMultMatrix(this->rigidBody->getTransform());

						// Draw axis
						ofDrawAxis(1.0f);

						// Draw model preview
						if (args.modelPreview) {
							auto& gridTexture = ofxRulr::Utils::getGridTexture();
							gridTexture.bind();
							{
								this->modelPreview.draw();
							}
							gridTexture.unbind();
						}

						auto modelViewMatrix = ofGetCurrentMatrix(ofMatrixMode::OF_MATRIX_MODELVIEW);

						// Draw frustum
						if (args.frustum) {
							args.drawArgs.onDrawFinal += [this, modelViewMatrix]() {
								ofPushMatrix();
								{
									ofLoadMatrix(modelViewMatrix);

									ofPushStyle();
									{
										auto color = this->color.get();
										ofSetColor(color);
										this->frustumPreview.drawWireframe();
										color.a = 100;
										ofSetColor(color);
										this->frustumPreview.drawFaces();
									}
									ofPopStyle();
								}
								ofPopMatrix();
							};
						}

						// Draw picture
						if (args.picture) {
							this->updatePicturePreviewWorld();

							args.drawArgs.onDrawFinal += [this, modelViewMatrix, args]() {
								ofSetDepthTest(false);
								ofPushMatrix();
								{
									ofLoadMatrix(modelViewMatrix);
									ofPushStyle();
									ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
									{
										ofSetColor(255.0f * this->parameters.deviceState.projection.color.red.get()
											, 255.0f * this->parameters.deviceState.projection.color.green.get()
											, 255.0f * this->parameters.deviceState.projection.color.blue.get()
											, args.pictureBrightness);
										this->lastPicturePreviewWorld.drawFaces();
									}
									ofDisableBlendMode();
									ofPopStyle();
								}
								ofPopMatrix();
								ofSetDepthTest(true);
							};
						}
					}
					ofPopMatrix();
				}

				// Draw center ray
				if (args.centerRay) {
					auto model = this->getModel();
					ofPushStyle();
					{
						auto ray = model.castRayWorldSpace({ 0.0, 0.0 });
						ofSetColor(this->color);
						ofDrawLine(ray.s
							, ray.s + ray.t * 100);
					}
					ofPopStyle();
				}

				// Draw truss line
				if (args.trussLine) {
					auto position = this->rigidBody->getPosition();
					ofPushStyle();
					{
						ofSetColor(100);
						ofDrawLine(position, position * glm::vec3(1, 0, 1) + glm::vec3(0, args.groundHeight, 0));
					}
					ofPopStyle();
				}
			}

			//----------
			string
				Laser::getLabelName() const
			{
				return "p" + ofToString(this->parameters.positionIndex.get()) + ":s" + ofToString(this->parameters.serialNumber.get());
			}

			//----------
			shared_ptr<Nodes::Item::RigidBody>
				Laser::getRigidBody()
			{
				return this->rigidBody;
			}

			//----------
			bool
				Laser::getIsAbnormal() const
			{
				if (!this->parameters.communications.hostnameOverride.get().empty()) {
					return true;
				}

				if (!this->parent->isCommunicationEnabled()) {
					return true;
				}

				if (this->pictureIsOutsideRange()) {
					return true;
				}

				return false;
			}

			//----------
			string
				Laser::getHostname() const
			{
				const auto& hostnameOverride = this->parameters.communications.hostnameOverride.get();
				if (!hostnameOverride.empty()) {
					return hostnameOverride;
				}
				else {
					return this->parameters.communications.hostname.get();
				}
			}

			//----------
			shared_ptr<OutgoingMessageRetry>
				Laser::createOutgoingMessageRetry() const
			{
				return make_shared<OutgoingMessageRetry>(this->getHostname()
					, std::chrono::milliseconds(this->parent->parameters.communications.retryDuration.get())
					, std::chrono::milliseconds(this->parent->parameters.communications.retryPeriod.get()));
			}

			//----------
			shared_ptr<OutgoingMessageOnce>
				Laser::createOutgoingMessageOnce() const
			{
				return make_shared<OutgoingMessageOnce>(this->getHostname());
			}

			//----------
			void
				Laser::shutdown()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/shutdown");
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);
				{
					auto state = this->parameters.deviceState.state.get();
					state.set(State::Shutdown);
					this->parameters.deviceState.state.set(state);
				}
				// Just to update the cached parameters (will send message twice also)
				this->pushState();
			}

			//----------
			future<void>
				Laser::pushState()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/device/setState");
					message->addInt32Arg(this->parameters.deviceState.state.get().toIndex());
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);

				this->sentDeviceParameters.state.set(this->parameters.deviceState.state.get());
				return future;
			}

			//----------
			future<void>
				Laser::pushLocalKeepAlive()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/device/keepAlive");
					message->addBoolArg(this->parameters.deviceState.localKeepAlive.get());
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);

				this->sentDeviceParameters.localKeepAlive.set(this->parameters.deviceState.localKeepAlive.get());
				return future;
			}

			//----------
			future<void>
				Laser::pushSource()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/device/setSource");
					message->addInt32Arg(this->parameters.deviceState.projection.source.get().toIndex());
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);

				this->sentDeviceParameters.projection.source.set(this->parameters.deviceState.projection.source.get());
				return future;
			}

			//----------
			future<void>
				Laser::pushColor()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/device/setColor");
					message->addFloatArg(this->parameters.deviceState.projection.color.red.get());
					message->addFloatArg(this->parameters.deviceState.projection.color.green.get());
					message->addFloatArg(this->parameters.deviceState.projection.color.blue.get());
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);

				this->sentDeviceParameters.projection.color.red.set(this->parameters.deviceState.projection.color.red.get());
				this->sentDeviceParameters.projection.color.green.set(this->parameters.deviceState.projection.color.green.get());
				this->sentDeviceParameters.projection.color.blue.set(this->parameters.deviceState.projection.color.blue.get());
				return future;
			}

			//----------
			future<void>
				Laser::pushTransform()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/device/setTransform");
					message->addFloatArg(this->parameters.deviceState.projection.transform.sizeX.get());
					message->addFloatArg(this->parameters.deviceState.projection.transform.sizeY.get());
					message->addFloatArg(this->parameters.deviceState.projection.transform.offsetX.get());
					message->addFloatArg(this->parameters.deviceState.projection.transform.offsetY.get());
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);

				this->sentDeviceParameters.projection.transform.sizeX.set(this->parameters.deviceState.projection.transform.sizeX.get());
				this->sentDeviceParameters.projection.transform.sizeY.set(this->parameters.deviceState.projection.transform.sizeY.get());
				this->sentDeviceParameters.projection.transform.offsetX.set(this->parameters.deviceState.projection.transform.offsetX.get());
				this->sentDeviceParameters.projection.transform.offsetY.set(this->parameters.deviceState.projection.transform.offsetY.get());
				return future;
			}

			//----------
			future<void>
				Laser::pushCircle()
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/device/setCircle");
					message->addFloatArg(this->parameters.deviceState.projection.circle.sizeX.get());
					message->addFloatArg(this->parameters.deviceState.projection.circle.sizeY.get());
					message->addFloatArg(this->parameters.deviceState.projection.circle.offsetX.get());
					message->addFloatArg(this->parameters.deviceState.projection.circle.offsetY.get());
					message->addFloatArg(this->parameters.deviceState.projection.circle.phase.get());
					message->addFloatArg(this->parameters.deviceState.projection.circle.frequency.get());
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);

				this->sentDeviceParameters.projection.circle.sizeX.set(this->parameters.deviceState.projection.circle.sizeX.get());
				this->sentDeviceParameters.projection.circle.sizeY.set(this->parameters.deviceState.projection.circle.sizeY.get());
				this->sentDeviceParameters.projection.circle.offsetX.set(this->parameters.deviceState.projection.circle.offsetX.get());
				this->sentDeviceParameters.projection.circle.offsetY.set(this->parameters.deviceState.projection.circle.offsetY.get());
				this->sentDeviceParameters.projection.circle.phase.set(this->parameters.deviceState.projection.circle.phase.get());
				this->sentDeviceParameters.projection.circle.frequency.set(this->parameters.deviceState.projection.circle.frequency.get());

				return future;
			}

			//----------
			void
				Laser::pushAll()
			{
				this->pushState();
				this->pushLocalKeepAlive();
				this->pushSource();
				this->pushColor();
				this->pushTransform();
				this->pushCircle();
			}

			//----------
			std::future<void>
				Laser::drawCircle(glm::vec2 center, float radius)
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/picture/circle");
					message->addFloatArg(center.x);
					message->addFloatArg(center.y);
					message->addFloatArg(radius);
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);
				return future;
			}

			//----------
			std::future<void>
				Laser::drawCalibrationBeam(const glm::vec2 & projectionPoint)
			{
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/picture/point");
					message->addFloatArg(projectionPoint.x);
					message->addFloatArg(projectionPoint.y);
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);
				return future;
			}

			//----------
			std::future<void>
				Laser::drawPicture(const vector<glm::vec2>& projectionPoints)
			{
				// check if any projectionPoints are invalid
				for (const auto& projectionPoint : projectionPoints) {
					if (abs(projectionPoint.x) > 1.0f || abs(projectionPoint.y) > 1.0f) {
						throw(ofxRulr::Exception("Picture is outside of limits"));
					}
				}

				if (projectionPoints.size() < 256) {
					throw(ofxRulr::Exception("Insufficient points for a projector picture"));
				}

				// Create the message
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/picture/picture");
					for (const auto& projectionPoint : projectionPoints) {
						message->addFloatArg(projectionPoint.x);
						message->addFloatArg(projectionPoint.y);
					}
				}

				// Route the message
				auto future = message->onSent.get_future();
				this->sendMessage(message);
				this->lastPictureSent = projectionPoints;
				this->lastPicturePreviewWorldStale = true;
				return future;
			}

			//----------
			vector<glm::vec2>
				Laser::renderWorldPoints(const vector<glm::vec3>& worldPoints, const vector<glm::vec2>& priorPicture) const
			{
				bool usePriors = priorPicture.size() == worldPoints.size();

				const auto& solverSettings = ofxRulr::Solvers::NavigateToWorldPoint::defaultSolverSettings();
				const auto& laserProjectorModel = this->getModel();

				vector<glm::vec2> results;

				if (usePriors) {
					// With priors
					size_t i = 0;
					for (auto& worldPoint : worldPoints) {
						auto initialGuess = this->lastPictureSent[i];
						if (glm::any(glm::isnan(initialGuess))
							|| initialGuess.x < -1 || initialGuess.x > 1
							|| initialGuess.y < -1 || initialGuess.y > 1) {
							if (i > 0) {
								initialGuess = results.back(); // use previous point
							}
							else {
								initialGuess = { 0, 0 };
							}
						}

						auto result = ofxRulr::Solvers::NavigateToWorldPoint::solve(worldPoint
							, laserProjectorModel
							, initialGuess
							, solverSettings);
						results.push_back(result.solution.point);

						i++;
					}
				}
				else {
					// Without priors
					for (auto& worldPoint : worldPoints) {
						auto result = ofxRulr::Solvers::NavigateToWorldPoint::solve(worldPoint
							, laserProjectorModel
							, glm::vec2(0, 0)
							, solverSettings);
						results.push_back(result.solution.point);
					}
				}

				return results;
			}

			//----------
			vector<glm::vec2>
				Laser::renderWorldPointsFast(const vector<glm::vec3>& worldPoints) const
			{
				// Perhaps this could be cached
				auto viewTransform = glm::inverse(this->rigidBody->getTransform());

				const auto halfFov = this->parameters.intrinsics.fov.get() / 2.0f * DEG_TO_RAD;

				vector<glm::vec2> picture;

				for (const auto& worldPoint : worldPoints) {
					auto point_view = ofxCeres::VectorMath::applyTransform(viewTransform, worldPoint);
					auto point_projected = glm::vec2(point_view.x, point_view.y) / point_view.z;
					
					auto tilt = atan(point_projected.y);
					auto pan = atan(point_projected.x * cos(tilt));

					picture.push_back({ pan / halfFov.x, tilt / halfFov.y });
				}

				return picture;
			}

			//----------
			std::future<void>
				Laser::drawWorldPoints(const vector<glm::vec3>& worldPoints)
			{
				auto picture = this->renderWorldPoints(worldPoints, this->getLastPicture());
				return this->drawPicture(picture);
			}

			//----------
			void
				Laser::sendMessage(shared_ptr<OutgoingMessage> message)
			{
				if (this->parent->sendMessage(message)) {
					this->isFrameNewTransmit.notify();
				}
			}

			//----------
			void
				Laser::processIncomingMessage(shared_ptr<IncomingMessage> message)
			{
				this->isFrameNewIncoming.notify();
			}

			//----------
			void
				Laser::processIncomingAck(shared_ptr<AckMessageIncoming> message)
			{
				this->isFrameNewAck.notify();
			}

			//----------
			Models::LaserProjector
				Laser::getModel() const
			{
				return Models::LaserProjector{
					this->rigidBody->getTransform()
					, this->parameters.intrinsics.fov.get()
					, this->parameters.intrinsics.fov2.get()
				};
			}

			//----------
			void
				Laser::rebuildModelPreview()
			{
				auto model = this->getModel();

				this->modelPreview.clear();

				auto planePrimitive = ofPlanePrimitive(2.0f, 2.0f, 9, 9);
				const auto & planeVertices = planePrimitive.getMesh().getVertices();
				vector<glm::vec3> vertices;
				vector<glm::vec2> textureCoordinates;
				for (auto& planeVertex : planeVertices) {
					auto ray = model.castRayObjectSpace({ planeVertex.x, planeVertex.y });
					vertices.push_back(ray.t);
					textureCoordinates.push_back(glm::vec2(planeVertex.x, planeVertex.y) * 0.5f + glm::vec2(0.5f, 0.5f));
				}
				this->modelPreview.addVertices(vertices);
				this->modelPreview.addTexCoords(textureCoordinates);
				this->modelPreview.addIndices(planePrimitive.getMesh().getIndices());
				this->modelPreview.setMode(planePrimitive.getMesh().getMode());
			}

			//----------
			void
				Laser::rebuildFrustumPreview()
			{
				auto model = this->getModel();

				this->frustumPreview.clear();
				this->frustumPreview.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);
				this->frustumPreview.addVertex({ 0, 0, 0 });

				vector<glm::vec2> projectionPoints{
					{ -1, -1 },
					{ -1, 0 },
					{ -1, 1 },
					{ 0, -1 },
					{ 1, -1 },
					{ 1, 0 },
					{ 1, 1 },
					{ 0, 1 },
					{ -1, 1 },
					{ -1, 0 },
					{ -1, -1 }
				};

				for (const auto& projectionPoint : projectionPoints) {
					auto ray = model.castRayObjectSpace(projectionPoint);
					auto vertex = ray.t * 100.0f;
					this->frustumPreview.addVertex(vertex);
				}
			}

			//----------
			const vector<glm::vec2>&
				Laser::getLastPicture() const
			{
				return this->lastPictureSent;
			}

			//----------
			void
				Laser::exportLastPicture(const std::filesystem::path& path) const
			{
				vector<float> values;
				for (const auto& point : this->lastPictureSent) {
					values.push_back(point.x);
					values.push_back(point.y);
				}
				nlohmann::json json = values;

				ofstream file(path.string(), ios::out);
				file << json.dump(4);
				file.close();
			}

			//----------
			bool
				Laser::pictureIsOutsideRange() const
			{
				for (auto point : this->lastPictureSent) {
					if (point.x < -1 || point.y > 1
						|| point.y < -1 || point.y > 1) {
						return true;
					}
				}

				return false;
			}

			//----------
			ofxCvGui::ElementPtr
				Laser::getDataDisplay()
			{
				auto stack = make_shared<ofxCvGui::Widgets::HorizontalStack>();

				// Add items to stack
				{
					// Laser number
					{
						auto element = make_shared<ofxCvGui::Element>();
						element->onDraw += [this](ofxCvGui::DrawArguments& args) {
							auto bounds = args.localBounds;
							bounds.height /= 2.0f;

							// Abnormal notice
							if (this->getIsAbnormal()) {
								ofPushStyle();
								{
									ofSetColor(100, 50, 50);
									ofFill();
									ofDrawRectangle(args.localBounds);
								}
								ofPopStyle();
							}

							ofxCvGui::Utils::drawText(this->getLabelName()
								, bounds
								, false
								, false);

							bounds.y += bounds.height;

							ofxCvGui::Utils::drawText(this->getHostname()
								, bounds
								, false
								, false);
						};
						stack->add(element);
					}

					// Acks
					{
						auto verticalStack = make_shared<ofxCvGui::Widgets::VerticalStack>();
						stack->add(verticalStack);
						{
							{
								auto heartbeat = make_shared<ofxCvGui::Widgets::Heartbeat>("Tx", [this]() {
									return this->isFrameNewTransmit.isFrameNew;
									});
								verticalStack->add(heartbeat);
							}
							{
								auto heartbeat = make_shared<ofxCvGui::Widgets::Heartbeat>("Rx", [this]() {
									return this->isFrameNewIncoming.isFrameNew;
									});
								verticalStack->add(heartbeat);
							}
							{
								auto heartbeat = make_shared<ofxCvGui::Widgets::Heartbeat>("Ack", [this]() {
									return this->isFrameNewAck.isFrameNew;
									});
								verticalStack->add(heartbeat);
							}
						}
					}

					// Inspect this
					{
						auto toggle = make_shared<ofxCvGui::Widgets::Toggle>("Inspect"
							, [this]() {
								return this->isBeingInspected();
							}
							, [this](const bool& selected) {
								if (selected) {
									ofxCvGui::inspect(this->shared_from_this());
								}
							});
						toggle->setDrawGlyph(u8"\uf002");
						stack->add(toggle);
					}

					// Inspect rigidBody
					{
						auto toggle = make_shared<ofxCvGui::Widgets::Toggle>("RigidBody"
							, [this]() {
								return this->rigidBody->isBeingInspected();
							}
							, [this](const bool& selected) {
								if (selected) {
									ofxCvGui::inspect(this->rigidBody);
								}
							});
						toggle->setDrawable([this](ofxCvGui::DrawArguments& args) {
							ofRectangle bounds;
							bounds.setFromCenter(args.localBounds.getCenter(), 32, 32);
							this->rigidBody->getIcon()->draw(bounds);
							});
						stack->add(toggle);
					}
				}

				return stack;
			}

			//----------
			void
				Laser::callbackIntrinsicsChange(glm::vec2&)
			{
				this->rebuildModelPreview();
				this->rebuildFrustumPreview();
			}

			//----------
			void
				Laser::updatePicturePreviewWorld()
			{
				if (this->lastPicturePreviewWorldStale) {
					this->lastPicturePreviewWorld.clear();

					auto model = this->getModel();

					this->lastPicturePreviewWorld.clear();
					this->lastPicturePreviewWorld.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);
					this->lastPicturePreviewWorld.addVertex({ 0, 0, 0 });

					if (!this->lastPictureSent.empty()) {
						for (const auto& projectionPoint : lastPictureSent) {
							auto ray = model.castRayObjectSpace(projectionPoint);
							auto vertex = ray.t * 100.0f;
							this->lastPicturePreviewWorld.addVertex(vertex);
						}

						// close it with the first point again
						const auto& projectionPoint = lastPictureSent.front();
						auto ray = model.castRayObjectSpace(projectionPoint);
						auto vertex = ray.t * 100.0f;
						this->lastPicturePreviewWorld.addVertex(vertex);
					}

					this->lastPicturePreviewWorldStale = false;
				}
			}
		}
	}
}
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
				ss << this->parameters.communications.address << " : (" << this->rigidBody->getPosition() << ") [" << this->parameters.intrinsics.centerOffset << "]";
				return ss.str();
			}

			//----------
			void
				Laser::update()
			{
				this->rigidBody->setName(ofToString(this->parameters.positionIndex.get()));
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
								this->parameters.communications.address.set(jsonSettings["Address"].get<int>());
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
				else {
					ofPushMatrix();
					{
						ofMultMatrix(this->rigidBody->getTransform());
						ofDrawAxis(1.0f);
					}
					ofPopMatrix();
				}

				// Draw center line
				if (args.centerLine) {
					ofPushMatrix();
					ofPushStyle();
					{
						ofMultMatrix(this->rigidBody->getTransform());
						ofSetColor(this->color);
						ofDrawLine({ 0, 0, 0 }, { 0, 0, 100 });
					}
					ofPopStyle();
					ofPopMatrix();
				}

				// Draw center offset line
				if (args.centerOffsetLine) {
					auto model = this->getModel();
					ofPushStyle();
					{
						auto ray = model.castRayWorldSpace(this->parameters.intrinsics.centerOffset);
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
			shared_ptr<Nodes::Item::RigidBody>
				Laser::getRigidBody()
			{
				return this->rigidBody;
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
					return this->parent->parameters.baseAddress.get() + ofToString(this->parameters.communications.address);
				}
			}

			//----------
			shared_ptr<OutgoingMessageRetry>
				Laser::createOutgoingMessageRetry() const
			{
				return make_shared<OutgoingMessageRetry>(this->getHostname()
					, std::chrono::milliseconds(this->parameters.communications.retryDuration.get())
					, std::chrono::milliseconds(this->parameters.communications.retryPeriod.get()));
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
				center += this->parameters.intrinsics.centerOffset.get();

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
				auto message = this->createOutgoingMessageRetry();
				{
					message->setAddress("/picture/picture");
					for (const auto& projectionPoint : projectionPoints) {
						message->addFloatArg(projectionPoint.x);
						message->addFloatArg(projectionPoint.y);
					}
				}
				auto future = message->onSent.get_future();
				this->sendMessage(message);
				this->lastPictureSent = projectionPoints;
				return future;
			}

			//----------
			std::future<void>
				Laser::drawWorldPoints(const vector<glm::vec3>& worldPoints)
			{
				bool usePriors = this->lastPictureSent.size() == worldPoints.size();

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
				
				return this->drawPicture(results);
			}

			//----------
			void
				Laser::sendMessage(shared_ptr<OutgoingMessage> message)
			{
				this->parent->sendMessage(message);
				this->isFrameNewTransmit.notify();
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
				};
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

							ofxCvGui::Utils::drawText("Position #" + ofToString(this->parameters.positionIndex)
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
		}
	}
}
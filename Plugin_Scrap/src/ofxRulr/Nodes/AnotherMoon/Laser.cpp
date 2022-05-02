#include "pch_Plugin_Scrap.h"
#include "Laser.h"
#include "Lasers.h"

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
				this->shutown();
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
				ss << this->parameters.settings.address << " : (" << this->rigidBody->getPosition() << ") [" << this->parameters.settings.centerOffset << "]";
				return ss.str();
			}

			//----------
			void
				Laser::update()
			{
				this->rigidBody->setName(ofToString(this->parameters.settings.address.get()));
				this->rigidBody->setColor(this->color);
				this->rigidBody->update();
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
			}

			//----------
			void
				Laser::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;
				inspector->addParameterGroup(this->parameters);
			}

			//----------
			void
				Laser::drawWorldStage()
			{
				this->rigidBody->drawWorldStage();

				// Draw center line
				if (ofxRulr::isActive(this, this->parameters.debug.draw.centerLine)) {
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
			}

			//----------
			shared_ptr<Nodes::Item::RigidBody>
				Laser::getRigidBody()
			{
				return this->rigidBody;
			}

			//----------
			void
				Laser::shutown()
			{
				ofxOscMessage msg;
				msg.setAddress("/shutdown");
				this->sendMessage(msg);
			}

			//----------
			void
				Laser::standby()
			{
				{
					ofxOscMessage msg;
					msg.setAddress("/standby");
					this->sendMessage(msg);
				}

				// Early LaserClient has spelling mistake - so we send this too
				{
					ofxOscMessage msg;
					msg.setAddress("/stanby");
					this->sendMessage(msg);
				}
			}

			//----------
			void
				Laser::run()
			{
				ofxOscMessage msg;
				msg.setAddress("/run");
				this->sendMessage(msg);
			}

			//----------
			void
				Laser::setBrightness(float value)
			{
				ofxOscMessage msg;
				msg.setAddress("/brightness");
				msg.addFloatArg(value);
				this->sendMessage(msg);
			}

			//----------
			void
				Laser::setSize(float value)
			{
				ofxOscMessage msg;
				msg.setAddress("/size");
				msg.addFloatArg(value);
				this->sendMessage(msg);
			}

			//----------
			void
				Laser::setSource(const Source& source)
			{
				int sourceIndex = 1;
				switch (source.get()) {
				case Source::Circle:
					sourceIndex = 0;
					break;
				case Source::USB:
					sourceIndex = 1;
					break;
				case Source::Memory:
					sourceIndex = 2;
					break;
				default:
					break;
				}

				ofxOscMessage msg;
				msg.setAddress("/source");
				msg.addInt32Arg(sourceIndex);
				this->sendMessage(msg);
			}

			//----------
			void
				Laser::drawCircle(glm::vec2 center, float radius)
			{
				center += this->parameters.settings.centerOffset.get();

				ofxOscMessage msg;
				msg.setAddress("/circle");
				msg.addFloatArg(center.x);
				msg.addFloatArg(center.y);
				msg.addFloatArg(radius);
				this->sendMessage(msg);
			}

			//----------
			string
				Laser::getHostname() const
			{
				return this->parent->parameters.baseAddress.get() + ofToString(this->parameters.settings.address);
			}

			//----------
			void
				Laser::sendMessage(const ofxOscMessage& msg)
			{
				if (this->oscSender) {
					if (this->oscSender->getHost() != this->getHostname()
						|| this->oscSender->getPort() != this->parent->parameters.remotePort.get()) {
						this->oscSender.reset();
					}
				}

				if (!this->oscSender) {
					this->oscSender = make_unique<ofxOscSender>();
					this->oscSender->setup(this->getHostname(), this->parent->parameters.remotePort.get());
				}

				// can put some error checking here in-between

				if (this->oscSender) {
					this->oscSender->sendMessage(msg);
				}
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
							ofxCvGui::Utils::drawText("Laser #" + ofToString(this->parameters.settings.address)
								, args.localBounds
								, false
								, false);
						};
						stack->add(element);
					}

					// Select this
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

					// Select this
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
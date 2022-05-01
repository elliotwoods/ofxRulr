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

			}

			//----------
			void
				Laser::drawWorldStage()
			{
				this->rigidBody->drawWorldStage();
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
		}
	}
}
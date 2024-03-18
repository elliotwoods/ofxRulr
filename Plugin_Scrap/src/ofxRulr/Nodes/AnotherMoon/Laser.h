#pragma once

#include "api_Plugin_Scrap.h"

#include "ofxRulr.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxOsc.h"
#include "ofxRulr/Models/LaserProjector.h"

#include "ofxRulr/Data/AnotherMoon/MessageRouter.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			struct PLUGIN_SCRAP_API_ENTRY IsFrameNew {
				void notify() {
					this->notifyFrameNew = true;
				}
				void update() {
					this->isFrameNew = this->notifyFrameNew;
					this->notifyFrameNew = false;
				}
				bool isFrameNew = false;
				bool notifyFrameNew = false;
			};

			class Lasers;

			class PLUGIN_SCRAP_API_ENTRY Laser
				: public Utils::AbstractCaptureSet::BaseCapture
				, public ofxCvGui::IInspectable
				, public enable_shared_from_this<Laser>
			{
			public:
				MAKE_ENUM(State
					, (Shutdown, Standby, Run)
					, ("Shutdown", "Standby", "Run"));

				MAKE_ENUM(Source
					, (Circle, USB, Memory)
					, ("Circle", "USB", "Memory"));

				struct DrawArguments {
					ofxRulr::DrawWorldAdvancedArgs& drawArgs;
					bool rigidBody;
					bool trussLine;
					bool centerRay;
					bool modelPreview;
					float groundHeight;
					bool frustum;
					bool picture;
					float pictureBrightness;
				};

				Laser();
				~Laser();
				void setParent(Lasers*);

				string getDisplayString() const override;

				void update();
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);
				void populateInspector(ofxCvGui::InspectArguments&);
				void drawWorldStage(const DrawArguments&);

				string getLabelName() const;

				string getHostname() const;
				shared_ptr<Item::RigidBody> getRigidBody();
				bool getIsAbnormal() const;

				shared_ptr<Data::AnotherMoon::OutgoingMessageRetry> createOutgoingMessageRetry() const;
				shared_ptr<Data::AnotherMoon::OutgoingMessageOnce> createOutgoingMessageOnce() const;

				void shutdown();

				std::future<void> pushState();
				std::future<void> pushLocalKeepAlive();
				std::future<void> pushSource();
				std::future<void> pushColor();
				std::future<void> pushTransform();
				std::future<void> pushCircle();

				void pushAll();

				std::future<void> drawCircle(glm::vec2 center, float radius);
				std::future<void> drawCalibrationBeam(const glm::vec2 & projectionPoint);
				std::future<void> drawPicture(const vector<glm::vec2>&);

				vector<glm::vec2> renderWorldPoints(const vector<glm::vec3>&, const vector<glm::vec2>& priorPicture = vector<glm::vec2>()) const;
				std::future<void> drawWorldPoints(const vector<glm::vec3>&);

				void sendMessage(shared_ptr<Data::AnotherMoon::OutgoingMessage>);
				void processIncomingMessage(shared_ptr<Data::AnotherMoon::IncomingMessage>);
				void processIncomingAck(shared_ptr<Data::AnotherMoon::AckMessageIncoming>);

				Models::LaserProjector getModel() const;

				void rebuildModelPreview();
				void rebuildFrustumPreview();

				const vector<glm::vec2>& getLastPicture() const;
				void exportLastPicture(const std::filesystem::path&) const;
				bool pictureIsOutsideRange() const;

				struct DeviceStateParameters : ofParameterGroup {
					ofParameter<State> state{ "State", State::Shutdown };
					ofParameter<bool> localKeepAlive{ "Local keep alive", false };

					struct : ofParameterGroup {
						ofParameter<Source> source{ "Source", Source::Memory };

						struct : ofParameterGroup {
							ofParameter<float> red{ "Red", 0.0f, 0.0f, 1.0f };
							ofParameter<float> green{ "Green", 0.0f, 0.0f, 1.0f };
							ofParameter<float> blue{ "Blue", 0.0f, 0.0f, 1.0f };
							PARAM_DECLARE("Color", red, green, blue);
						} color;

						struct : ofParameterGroup {
							ofParameter<float> sizeX{ "Size X", 1.0f, 0.0f, 1.0f };
							ofParameter<float> sizeY{ "Size Y", 1.0f, 0.0f, 1.0f };
							ofParameter<float> offsetX{ "Offset X", 0, -1.0f, 1.0f };
							ofParameter<float> offsetY{ "Offset Y", 0, -1.0f, 1.0f };
							PARAM_DECLARE("Transform", sizeX, sizeY, offsetX, offsetY);
						} transform;

						struct : ofParameterGroup {
							ofParameter<float> sizeX{ "Size X", 1.0f, 0.0f, 1.0f };
							ofParameter<float> sizeY{ "Size Y", 1.0f, 0.0f, 1.0f };
							ofParameter<float> offsetX{ "Offset X", 0, -1.0f, 1.0f };
							ofParameter<float> offsetY{ "Offset Y", 0, -1.0f, 1.0f };
							ofParameter<float> phase{ "Phase", 90.0f, 0.0f, 1.0f };
							ofParameter<float> frequency{ "Frequency", 50.0f, 0.0f, 100.0f };
							PARAM_DECLARE("Circle", sizeX, sizeY, offsetX, offsetY, phase, frequency);
						} circle;

						PARAM_DECLARE("Projection", source, color, transform);
					} projection;

					PARAM_DECLARE("Device state", state, localKeepAlive, projection);
				};

				struct : ofParameterGroup {
					ofParameter<int> serialNumber{ "Serial number", 1 };
					ofParameter<int> positionIndex{ "Position index", 1 };

					struct : ofParameterGroup {
						ofParameter<string> hostname{ "IP Address", "10.0.1.1" };

						struct : ofParameterGroup {
							ofParameter<WhenActive> enabled{ "Enabled", WhenActive::Selected };
							ofParameter<int> period{ "Period [ms]", 500 };
							PARAM_DECLARE("Keep alive", enabled, period);
						} keepAlive;

						ofParameter<string> hostnameOverride{ "Hostname override", "" };

						PARAM_DECLARE("Communications", hostname, keepAlive, hostnameOverride);
					} communications;

					struct : ofParameterGroup {
						ofParameter<glm::vec2> fov{ "FOV", {30, 30} };
						ofParameter<glm::vec2> fov2{ "FOV 2nd order", {0, 0} };
						PARAM_DECLARE("Settings", fov, fov2);
					} intrinsics;

					DeviceStateParameters deviceState;
					
					PARAM_DECLARE("Laser", serialNumber, positionIndex, communications, intrinsics, deviceState);
				} parameters;

			protected:
				ofxCvGui::ElementPtr getDataDisplay() override;

				void callbackIntrinsicsChange(glm::vec2&);
				void updatePicturePreviewWorld();

				Lasers* parent;

				shared_ptr<Item::RigidBody> rigidBody = make_shared<Item::RigidBody>();

				DeviceStateParameters sentDeviceParameters;
				chrono::system_clock::time_point lastKeepAliveSent{};

				IsFrameNew isFrameNewAck;
				IsFrameNew isFrameNewIncoming;
				IsFrameNew isFrameNewTransmit;

				vector<glm::vec2> lastPictureSent;
				ofMesh lastPicturePreviewWorld;
				bool lastPicturePreviewWorldStale = true;

				ofMesh modelPreview;
				ofMesh frustumPreview;
			};
		}
	}
}
#include "pch_RulrNodes.h"
#include "Camera.h"

#include "ofxCvGui.h"

using namespace ofxCvGui;
using namespace ofxMachineVision;
using namespace cv;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Camera::Camera() : View(true) {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			Camera::~Camera() {
				this->closeDevice();
			}

			//----------
			void Camera::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->showSpecification.set("Show specification", false);
				this->showFocusLine.set("Show focus line", true);

				this->grabber = make_shared<ofxMachineVision::Grabber::Simple>();
				this->grabber->onNewFrameReceived += [this](shared_ptr<Frame> frame) {
					this->onNewFrame.notifyListeners(move(frame));
				};
				this->placeholderPanel = make_shared<Panels::Groups::Strip>();
				this->cameraOpenPanel = make_shared<ofxCvGui::Panels::Widgets>();

				this->buildGrabberPanel();
				this->rebuildPanel();

				this->onDrawObject += [this]() {
					if (this->grabber->getIsDeviceOpen()) {
						auto & grabberTexture = this->getGrabber()->getTexture();
						if (grabberTexture.isAllocated()) {
							this->getViewInObjectSpace().drawOnNearPlane(*this->getGrabber());
						}
					}
				};
			}

			//----------
			string Camera::getTypeName() const {
				return "Item::Camera";
			}

			//----------
			void Camera::update() {
				this->grabber->update();

				if (this->showFocusLine) {
					if (this->grabber->isFrameNew()) {
						auto frame = this->grabber->getFrame();
						const auto & pixels = frame->getPixels();
						if (pixels.isAllocated()) {
							auto middleRow = pixels.getData() + pixels.getWidth() * pixels.getNumChannels() * pixels.getHeight() / 2;

							this->focusLineGraph.clear();
							this->focusLineGraph.setMode(OF_PRIMITIVE_LINE_STRIP);
							for (int i = 0; i < pixels.getWidth(); i++) {
								this->focusLineGraph.addVertex(ofVec3f(i, *middleRow, 0));
								middleRow += pixels.getNumChannels();
							}
						}
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr Camera::getPanel() {
				return this->placeholderPanel;
			}

			//----------
			void Camera::serialize(Json::Value & json) {
				auto & jsonDevice = json["device"];
				{
					jsonDevice["width"] = this->getWidth();
					jsonDevice["height"] = this->getHeight();

				}

				this->buildCachedInitialisationSettings();
				json["cachedInitialisationSettings"] = this->cachedInitialisationSettings;
			}

			//----------
			void Camera::deserialize(const Json::Value & json) {
				const auto & jsonDevice = json["device"];
				{
					//take in the saved resolution, this will be overwritten when the device is open and running
					this->setWidth(jsonDevice["width"].asFloat());
					this->setHeight(jsonDevice["height"].asFloat());
				}

				this->cachedInitialisationSettings = json["cachedInitialisationSettings"];
				if (!this->grabber->getDevice()) {
					if (this->cachedInitialisationSettings.isMember("deviceType")) {
						this->setDevice(this->cachedInitialisationSettings["deviceType"].asString());
					}
				}
				if (this->grabber->getDevice()) {
					if (this->cachedInitialisationSettings["isOpen"].asBool()) {
						try {
							this->openDevice();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}
				}
			}

			//----------
			void Camera::setDevice(const string & deviceTypeName) {
				if (this->grabber->getDeviceTypeName() == deviceTypeName) {
					//do nothing
					return;
				}

				auto factory = ofxMachineVision::Device::FactoryRegister::X().get(deviceTypeName);
				shared_ptr<Device::Base> device;
				if (factory) {
					device = factory->makeUntyped();
					this->setDevice(device);
				}
				else {
					this->clearDevice();
				}
			}

			//----------
			void Camera::setDevice(DevicePtr device, shared_ptr<ofxMachineVision::Device::Base::InitialisationSettings> initialisationSettings) {
				this->grabber->setDevice(device);
				if (device) {
					if (!initialisationSettings) {
						initialisationSettings = this->grabber->getDefaultInitialisationSettings();
						this->applyAnyCachedInitialisationSettings(initialisationSettings);
					}
					this->initialisationSettings = initialisationSettings;
				}
				this->rebuildPanel();
			}

			//----------
			void Camera::clearDevice() {
				this->grabber->clearDevice();
				this->rebuildPanel();
			}

			//----------
			void Camera::openDevice() {
				auto device = this->grabber->getDevice();
				if (device) {
					Utils::ScopedProcess scopedProcess("Opening grabber device");
					this->grabber->open(this->initialisationSettings);
					if (!this->grabber->getIsDeviceOpen()) {
						throw(ofxRulr::Exception("Cannot open device of type [" + device->getTypeName() + "]"));
					}
					this->grabber->startCapture();
					if (!this->grabber->getIsDeviceOpen()) {
						throw(ofxRulr::Exception("Cannot start capture on device of type [" + device->getTypeName() + "]"));
					}

					//this block of code should be a bit safer.
					//it can happen that it's unclear to the user whether the Camera's width/height are valid or not
					auto width = this->grabber->getWidth();
					auto height = this->grabber->getHeight();
					if (width == 0 || height == 0) {
						width = grabber->getCaptureWidth();
						height = grabber->getCaptureHeight();
					}
					if (width != 0 && height != 0) {
						this->setWidth(width);
						this->setHeight(height);
						this->markViewDirty(); // size will have changed
					}
					else {
						ofSystemAlertDialog("Warning : Camera image size is not yet valid");
					}

					const auto & deviceSpecification = this->getGrabber()->getDeviceSpecification();
					this->grabberPanel->setCaption(deviceSpecification.getManufacturer() + " : " + deviceSpecification.getModelName());

					scopedProcess.end();
				}
				else {
					throw(ofxRulr::Exception("Cannot open device until one is set."));
				}
				this->rebuildPanel();
			}

			//----------
			void Camera::closeDevice() {
				if (this->grabber) {
					ofxCvGui::Utils::drawProcessingNotice("Closing grabber device...");
					grabber->close();
					this->rebuildPanel();
				}
			}

			//----------
			shared_ptr<Grabber::Simple> Camera::getGrabber() {
				return this->grabber;
			}

			//----------
			shared_ptr<ofxMachineVision::Frame> Camera::getFrame() {
				if (!this->grabber) {
					return nullptr;
				}
				else {
					return this->grabber->getFrame();
				}
			}

			//----------
			shared_ptr<ofxMachineVision::Frame> Camera::getFreshFrame() {
				if (!this->grabber) {
					return nullptr;
				}
				else {
					return this->grabber->getFreshFrame();
				}
			}

			//----------
			void Camera::rebuildPanel() {
				this->placeholderPanel->clear();
				if (this->grabber->getIsDeviceOpen()) {
					this->placeholderPanel->add(this->grabberPanel);
				}
				else {
					this->rebuildOpenCameraPanel();
					this->placeholderPanel->add(this->cameraOpenPanel);
				}
				ofxCvGui::InspectController::X().refresh(this);
			}

			//----------
			void Camera::buildGrabberPanel() {
				this->grabberPanel = ofxCvGui::Panels::makeBaseDraws(*this->grabber);
				this->grabberPanel->onDraw += [this](ofxCvGui::DrawArguments & args) {
					if (this->showSpecification) {
						stringstream status;
						status << "Device ID : " << this->getGrabber()->getDeviceID() << endl;
						status << endl;
						status << this->getGrabber()->getDeviceSpecification().toString() << endl;

						ofDrawBitmapStringHighlight(status.str(), 30, 90, ofColor(0x46, 200), ofColor::white);
					}
				};
				this->grabberPanel->onDrawImage += [this](DrawImageArguments & args) {
					if (this->showFocusLine) {
						ofPushMatrix();
						ofPushStyle();

						ofTranslate(0, args.drawBounds.width / 2.0f);
						ofSetColor(100, 255, 100);
						ofDrawLine(0, 0, args.drawBounds.width, 0);

						ofTranslate(0, +128);
						ofScale(1.0f, -1.0f);
						ofSetColor(0, 0, 0);
						ofSetLineWidth(2.0f);
						this->focusLineGraph.draw();
						ofSetLineWidth(1.0f);
						ofSetColor(255, 100, 100);
						this->focusLineGraph.draw();

						ofPopStyle();
						ofPopMatrix();
					}

					//crosshair
					ofPushStyle();
					ofSetLineWidth(1);
					ofSetColor(255);
					ofPushMatrix();
					ofTranslate(args.drawBounds.getCenter());
					ofDrawLine(-10, 0, 10, 0);
					ofDrawLine(0, -10, 0, 10);
					ofPopMatrix();
					ofPopStyle();
				};
			}

			//----------
			void Camera::rebuildOpenCameraPanel() {
				this->cameraOpenPanel->clear();

				this->cameraOpenPanel->addTitle("Select device type:");
				{
					auto & factories = ofxMachineVision::Device::FactoryRegister::X();
					for (auto factory : factories) {
						//add section title
						this->cameraOpenPanel->addTitle(factory.first, Widgets::Title::H3);

						//add buttons for listed devices
						{
							try {
								auto listDevicesDevice = factory.second->makeUntyped();
								auto foundDevices = listDevicesDevice->listDevices();
								for (auto foundDevice : foundDevices) {
									auto button = this->cameraOpenPanel->addButton(foundDevice.manufacturer + "\n" + foundDevice.model,
										[this, factory, foundDevice] {
										auto device = factory.second->makeUntyped();
										this->setDevice(device, foundDevice.initialisationSettings);
									});
									button->setHeight(50.0f);
								}
							}
							RULR_CATCH_ALL_TO_ERROR;
						}

						//add a button for devices not returned by listDevices
						this->cameraOpenPanel->addButton("Unknown " + factory.first,
							[this, factory]() {
							auto device = factory.second->makeUntyped();
							this->setDevice(device);
						});

					}
				}

				this->cameraOpenPanel->addSpacer();

				auto device = this->grabber->getDevice();
				if (device) {
					this->cameraOpenPanel->addTitle("Initialisation settings:");
					if (this->initialisationSettings) {
						this->cameraOpenPanel->addParameterGroup(*this->initialisationSettings);
					}

					auto openButton = this->cameraOpenPanel->addButton("Open", [this]() {
						try {
							this->openDevice();
						}
						RULR_CATCH_ALL_TO_ALERT;
					});
					openButton->setHeight(100.0f);
				}

				this->cameraOpenPanel->arrange();
			}

			//----------
			template<typename DataType, typename WidgetType>
			bool addParameterWidget(shared_ptr<ofxCvGui::Panels::Inspector> inspector
				, shared_ptr<AbstractParameter> parameter
				, shared_ptr<ofxMachineVision::Grabber::Simple> grabber) {
				auto typedOfParameter = parameter->getParameterTyped<DataType>();
				if (typedOfParameter) {
					auto widget = make_shared<WidgetType>(*typedOfParameter);

					auto units = parameter->getUnits();
					if (!units.empty()) {
						widget->setCaption(widget->getCaption() + " [" + units + "]");
					}

					//we don't use weak pointers here, just presume that we should never arrive here if grabber closed / param disappeared
					widget->onValueChange += [grabber, parameter](const DataType &) {
						grabber->syncToDevice(*parameter);
					};

					inspector->add(widget);
					return true;
				}
				else {
					return false;
				}
			}

			//----------
			void Camera::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;

				inspector->add(new Widgets::Toggle(this->showSpecification));
				inspector->add(new Widgets::Toggle(this->showFocusLine));

				inspector->add(new Widgets::Title("Device", Widgets::Title::H2));
				inspector->add(new Widgets::LiveValue<string>("Device Type", [this]() {
					return this->grabber->getDeviceTypeName();
				}));
				inspector->add(new Widgets::Button("Close device", [this]() {
					this->closeDevice();
				}));
				inspector->add(new Widgets::Button("Clear device", [this]() {
					this->clearDevice();
				}));
				inspector->add(new Widgets::LiveValueHistory("Fps [Hz]", [this]() {
					return this->grabber->getFps();
				}, true));
				inspector->add(new Widgets::LiveValueHistory("Timestamp [s]", [this]() {
					return (float)this->grabber->getLastTimestamp().count() / 1e9;
				}, false));
				inspector->add(new Widgets::LiveValueHistory("Frame index", [this]() {
					return this->grabber->getLastFrameIndex();
				}, false));

				inspector->add(new Widgets::Title("Properties", Widgets::Title::H2));
				auto grabber = this->getGrabber();
				if (grabber) {
					//Add device parameters
					{
						const auto & deviceParameters = grabber->getDeviceParameters();
						for (const auto & parameter : deviceParameters) {
							//try and add the parameter to the inspector
							if (addParameterWidget<float, Widgets::Slider>(inspector, parameter, grabber))	{}
							else if (addParameterWidget<int, Widgets::EditableValue<int>>(inspector, parameter, grabber)) {}
							else if (addParameterWidget<bool, Widgets::Toggle>(inspector, parameter, grabber)) {}
							else if (addParameterWidget<string, Widgets::EditableValue<string>>(inspector, parameter, grabber)) {}
						}
					}


					if (grabber->getDeviceSpecification().supports(ofxMachineVision::CaptureSequenceType::OneShot)) {
						inspector->add(MAKE(Widgets::Button, "Take Photo", [this]() {
							Utils::ScopedProcess scopedProcess("Take Photo");
							this->getGrabber()->singleShot();
							scopedProcess.end();
						}, ' '));
					}
				}
			}

			//----------
			void Camera::buildCachedInitialisationSettings() {
				this->cachedInitialisationSettings.clear();

				if (this->initialisationSettings) {
					this->cachedInitialisationSettings["deviceType"] = this->grabber->getDeviceTypeName();
					this->cachedInitialisationSettings["isOpen"] = this->grabber->getIsDeviceOpen();
					Utils::Serializable::serialize(this->cachedInitialisationSettings["content"], *this->initialisationSettings);
				}
			}

			//----------
			void Camera::applyAnyCachedInitialisationSettings(shared_ptr<ofxMachineVision::Device::Base::InitialisationSettings> initialisationSettings) {
				auto cachedDeviceType = this->cachedInitialisationSettings["deviceType"].asString();
				if (initialisationSettings
					&& this->grabber->getDeviceTypeName() == cachedDeviceType) {
					Utils::Serializable::deserialize(this->cachedInitialisationSettings["content"], *initialisationSettings);
				}
			}
		}
	}
}
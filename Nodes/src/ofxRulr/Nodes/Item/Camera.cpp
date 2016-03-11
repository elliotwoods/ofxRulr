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
			void Camera::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->placeholderView = make_shared<Panels::Groups::Grid>();

				this->deviceIndex.set("Device index", 0);
				this->deviceTypeName.set("Device type name", "uninitialised"); // this should be different from the setDevice("") call below

				this->showSpecification.set("Show specification", false);
				this->showFocusLine.set("Show focus line", true);
				this->exposure.set("Exposure [us]", 500.0f, 0.0f, 1000000.0f);
				this->gain.set("Gain", 0.5f, 0.0f, 1.0f);
				this->focus.set("Focus ", 0.5f, 0.0f, 1.0f);
				this->sharpness.set("Sharpness", 0.5f, 0.0f, 1.0f);

				this->exposure.addListener(this, &Camera::exposureCallback);
				this->gain.addListener(this, &Camera::gainCallback);
				this->focus.addListener(this, &Camera::focusCallback);
				this->sharpness.addListener(this, &Camera::sharpnessCallback);

				this->grabber = make_shared<ofxMachineVision::Grabber::Simple>();

				//this sets up the GUI for selecting a device
				this->setDevice("");

				this->onDrawObject += [this]() {
					if (this->grabber->getIsDeviceOpen()) {
						this->getViewInObjectSpace().drawOnNearPlane(* this->getGrabber());
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
						frame->lockForReading();
						const auto & pixels = frame->getPixels();
						if (pixels.isAllocated()) {
							auto middleRow = pixels.getData() + pixels.getWidth() * pixels.getNumChannels() * pixels.getHeight() / 2;

							this->focusLineGraph.clear();
							this->focusLineGraph.setMode(OF_PRIMITIVE_LINE_STRIP);
							for (int i = 0; i<pixels.getWidth(); i++) {
								this->focusLineGraph.addVertex(ofVec3f(i, *middleRow, 0));
								middleRow += pixels.getNumChannels();
							}
						}
						frame->unlock();
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr Camera::getView() {
				return this->placeholderView;
			}

			//----------
			void Camera::serialize(Json::Value & json) {
				auto & jsonDevice = json["device"];
				{
					Utils::Serializable::serialize(this->deviceTypeName, jsonDevice);
					Utils::Serializable::serialize(this->deviceIndex, jsonDevice);
					jsonDevice["width"] = this->getWidth();
					jsonDevice["height"] = this->getHeight();
					
				}

				auto & jsonSettings = json["properties"];
				{
					Utils::Serializable::serialize(this->exposure, jsonSettings);
					Utils::Serializable::serialize(this->gain, jsonSettings);
					Utils::Serializable::serialize(this->focus, jsonSettings);
					Utils::Serializable::serialize(this->sharpness, jsonSettings);
				}
			}

			//----------
			void Camera::deserialize(const Json::Value & json) {
				auto & jsonDevice = json["device"];
				{
					this->setDeviceIndex(jsonDevice[this->deviceIndex.getName()].asInt());
					this->setDevice(jsonDevice[this->deviceTypeName.getName()].asString());
					
					//take in the saved resolution, this will be overwritten when the device is open and running
					this->viewInObjectSpace.setWidth(jsonDevice["width"].asFloat());
					this->viewInObjectSpace.setHeight(jsonDevice["height"].asFloat());
				}

				auto & jsonSettings = json["properties"];
				{
					Utils::Serializable::deserialize(this->exposure, jsonSettings);
					Utils::Serializable::deserialize(this->gain, jsonSettings);
					Utils::Serializable::deserialize(this->focus, jsonSettings);
					Utils::Serializable::deserialize(this->sharpness, jsonSettings);
					
					this->setAllGrabberProperties();
				}
			}

			//----------
			void Camera::setDeviceIndex(int deviceIndex) {
				if (this->deviceIndex.get() == deviceIndex) {
					//do nothing
					return;
				}

				this->deviceIndex = deviceIndex;
				if (this->grabber->getIsDeviceOpen()) {
					//if we have a device open, let's reopen it
					this->setDevice(this->grabber->getDevice());
				}
			}

			//----------
			void Camera::setDevice(const string & deviceTypeName) {
				if (this->deviceTypeName.get() == deviceTypeName) {
					//do nothing
					return;
				}

				this->deviceTypeName = deviceTypeName;
				auto factory = ofxMachineVision::Device::FactoryRegister::X().get(deviceTypeName);
				shared_ptr<Device::Base> device;
				if (factory) {
					device = factory->makeUntyped();
				}
				this->setDevice(device); //setting to null is also valid
			}

			//----------
			void Camera::setDevice(DevicePtr device) {
				this->grabber->setDevice(device);

				if (device) {
					try
					{
						this->grabber->open(this->deviceIndex);
						if (!this->grabber->getIsDeviceOpen()) {
							throw(ofxRulr::Exception("Cannot open device of type [" + device->getTypeName() + "] at deviceIndex [" + ofToString(this->deviceIndex) + "]"));
						}
						this->grabber->startCapture();
						if (!this->grabber->getIsDeviceOpen()) {
							throw(ofxRulr::Exception("Cannot start capture on device of type [" + device->getTypeName() + "] at deviceIndex [" + ofToString(this->deviceIndex) + "]"));
						}

						//this block of code should be a bit safer.
						//it can happen that it's unclear to the user whether the Camera's width/height are valid or not
						auto width = this->grabber->getWidth();
						auto height = this->grabber->getHeight();
						if (width == 0 || height == 0) {
							width = grabber->getSensorWidth();
							height = grabber->getSensorHeight();
						}
						if (width != 0 && height != 0) {
							this->setWidth(width);
							this->setHeight(height);
							this->markViewDirty(); // size will have changed
						}
						else {
							ofSystemAlertDialog("Warning : Camera image size is not yet valid");
						}
						

						this->setAllGrabberProperties();

						auto cameraView = ofxCvGui::Panels::make(*this->grabber);
						cameraView->onDraw += [this](ofxCvGui::DrawArguments & args) {
							if (this->showSpecification) {
								stringstream status;
								status << "Device ID : " << this->getGrabber()->getDeviceID() << endl;
								status << endl;
								status << this->getGrabber()->getDeviceSpecification().toString() << endl;

								ofDrawBitmapStringHighlight(status.str(), 30, 90, ofColor(0x46, 200), ofColor::white);
							}
						};
						cameraView->onDrawCropped += [this](DrawCroppedArguments & args) {
							if (this->showFocusLine) {
								ofPushMatrix();
								ofPushStyle();

								ofTranslate(0, args.drawSize.y / 2.0f);
								ofSetColor(100, 255, 100);
								ofDrawLine(0, 0, args.drawSize.x, 0);

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
							ofTranslate(args.drawSize.x / 2.0f, args.drawSize.y / 2.0f);
							ofDrawLine(-10, 0, 10, 0);
							ofDrawLine(0, -10, 0, 10);
							ofPopMatrix();
							ofPopStyle();
						};

						const auto & deviceSpecification = this->getGrabber()->getDeviceSpecification();
						cameraView->setCaption(deviceSpecification.getManufacturer() + " : " + deviceSpecification.getModelName());

						this->placeholderView->clear();
						this->placeholderView->add(cameraView);
					}
					RULR_CATCH_ALL_TO({
						this->deviceTypeName = "";
						ofSystemAlertDialog(e.what());
					});
				}
				else {
					//there is no device, and we want to setup a view to select the device
					auto cameraSelectorView = make_shared<Panels::Scroll>();
					cameraSelectorView->add(new Widgets::EditableValue<int>(this->deviceIndex));
					cameraSelectorView->add(new Widgets::Title("Select device type:", Widgets::Title::Level::H3));

					auto & factories = ofxMachineVision::Device::FactoryRegister::X();
					for (auto factory : factories) {
						auto makeButton = make_shared<Widgets::Button>(factory.first, [this, factory]() {
							this->setDevice(factory.first);
						});
						cameraSelectorView->add(makeButton);
					}

					this->placeholderView->clear();
					this->placeholderView->add(cameraSelectorView);
				}
			}

			//----------
			void Camera::clearDevice() {
				this->setDevice(DevicePtr());
				this->deviceTypeName.set("uninitialised");
			}

			//----------
			void Camera::reopenDevice() {
				this->setDevice(this->getGrabber()->getDevice());
			}

			//----------
			shared_ptr<Grabber::Simple> Camera::getGrabber() {
				return this->grabber;
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
			void Camera::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				inspector->add(new Widgets::Toggle(this->showSpecification));
				inspector->add(new Widgets::Toggle(this->showFocusLine));

				inspector->add(new Widgets::Title("Device", Widgets::Title::H2));
				inspector->add(new Widgets::LiveValue<string>("Device Type", [this]() {
					return this->deviceTypeName;
				}));
				inspector->add(new Widgets::EditableValue<int>("Device Index", [this]() {
					return this->deviceIndex;
				}, [this](const string & deviceIndexSelection) {
					this->deviceIndex = ofToInt(deviceIndexSelection);
					this->reopenDevice();
				}));
				inspector->add(new Widgets::Button("Clear device", [this]() {
					this->clearDevice();
				}));
				inspector->add(new Widgets::LiveValueHistory("Fps [Hz]", [this]() {
					return this->grabber->getFps();
				}, true));
				inspector->add(new Widgets::LiveValueHistory("Timestamp [us]", [this]() {
					return this->grabber->getLastTimestamp();
				}, false));
				inspector->add(new Widgets::LiveValueHistory("Frame index", [this]() {
					return this->grabber->getLastFrameIndex();
				}, false));

				inspector->add(new Widgets::Title("Properties", Widgets::Title::H2));
				auto grabber = this->getGrabber();
				if (grabber) {
					if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Exposure)) {
						auto exposureSlider = new Widgets::Slider(this->exposure);
						exposureSlider->addIntValidator();
						inspector->add(exposureSlider);
					}
					if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Gain)) {
						inspector->add(new Widgets::Slider(this->gain));
					}
					if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Focus)) {
						inspector->add(new Widgets::Slider(this->focus));
					}
					if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_Sharpness)) {
						inspector->add(new Widgets::Slider(this->sharpness));
					}
					if (grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_OneShot)) {
						inspector->add(MAKE(Widgets::Button, "Take Photo", [this]() {
							Utils::ScopedProcess scopedProcess("Take Photo");
							this->getGrabber()->singleShot();
							scopedProcess.end();
						}, ' '));
					}
				}
			}

			//----------
			void Camera::setAllGrabberProperties() {
				auto deviceSpecification = this->grabber->getDeviceSpecification();

				if (deviceSpecification.supports(Feature::Feature_Exposure)) {
					this->grabber->setExposure(this->exposure);
				}
				if (deviceSpecification.supports(Feature::Feature_Gain)) {
					this->grabber->setGain(this->gain);
				}
				if (deviceSpecification.supports(Feature::Feature_Focus)) {
					this->grabber->setFocus(this->focus);
				}
				if (deviceSpecification.supports(Feature::Feature_Sharpness)) {
					this->grabber->setSharpness(this->sharpness);
				}
			}

			//----------
			void Camera::exposureCallback(float & value) {
				this->grabber->setExposure(value);
			}

			//----------
			void Camera::gainCallback(float & value) {
				this->grabber->setGain(value);
			}

			//----------
			void Camera::focusCallback(float & value) {
				this->grabber->setFocus(value);
			}

			//----------
			void Camera::sharpnessCallback(float & value) {
				this->grabber->setSharpness(value);
			}
		}
	}
}
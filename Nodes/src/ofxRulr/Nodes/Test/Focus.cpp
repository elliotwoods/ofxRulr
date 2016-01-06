#include "Focus.h"

#include "ofxRulr/Nodes/Item/Camera.h"

#include "ofxCvMin.h"
#include "ofxCvGui/Panels/ElementHost.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/EditableValue.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/Indicator.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/MultipleChoice.h"
#include "ofxCvGui/Widgets/Slider.h"

#include "ofxAssets.h"

using namespace ofxCvGui;
using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace Test {
			//----------
			Focus::Focus() {
				RULR_NODE_INIT_LISTENER;
			}
			
			//----------
			string Focus::getTypeName() const {
				return "Test::Focus";
			}
			
			//----------
			void Focus::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				
				auto cameraInput = this->addInput<Item::Camera>();
				{
					cameraInput->onNewConnection += [this](shared_ptr<Item::Camera> camera) {
						this->connect(camera->getGrabber());
					};
					cameraInput->onDeleteConnection += [this](shared_ptr<Item::Camera> camera) {
						if(camera) {
							this->disconnect(camera->getGrabber());
						}
					};
				}
				
				auto view = make_shared<Panels::Draws>(this->preview);
				{
					auto valueHistory = Widgets::LiveValueHistory::make("Focus value", [this](){
						float value;
						this->resultMutex.lock();
						{
							value = this->result.value;
						}
						this->resultMutex.unlock();
						return value;
					});
					valueHistory->addListenersToParent(view, true);
					this->widget = valueHistory; // we keep the shared_ptr
					
					view->onDraw += [this](DrawArguments & args) {
						if (!this->getRunFinderEnabled()) {
							ofxCvGui::Utils::drawText("Select this node and connect active camera.", args.localBounds);
						}
					};
					this->view = view;
				}

				this->preview.allocate(16,16);
				this->preview.begin();
				ofClear(0, 0);
				this->preview.end();
				
				this->activewhen.set("Active when", 0, 0, 1);
				this->blurSize.set("Blur size", 5, 1, 50);
				this->highValue.set("High value", 0.05, 0, 1);
				this->lowValue.set("Low value", 0.05, 0, 1);
				
				this->updateProcessSettings();
			}
			
			//----------
			void Focus::update() {
				this->updateProcessSettings();
				
				{
					lock_guard<mutex> lock(this->resultMutex);
					if(this->result.isFrameNew) {
						this->result.highFrequency.update();
						this->result.lowFrequency.update();
						
						if(this->preview.getWidth() != this->result.width || this->preview.getHeight() != this->result.height) {
							this->preview.allocate(this->result.width, this->result.height, GL_RGBA);
						}
						
						//--
						//Fill fbo
						//--
						//
						this->preview.begin();
						auto & shader = ofxAssets::shader("focusFinder");
						shader.begin();
						shader.setUniformTexture("highFrequency", this->result.highFrequency, 0);
						shader.setUniformTexture("lowFrequency", this->result.lowFrequency, 1);
						
						this->result.highFrequency.draw(0, 0); //draw something with texture coordinates (goes into first texture slow)
						
						shader.end();
						this->preview.end();
						//
						//--
						
						this->result.isFrameNew = false;
					}
				}
			}
			
			//----------
			void Focus::populateInspector(InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				auto activeWhenWidget = Widgets::MultipleChoice::make("Active");
				activeWhenWidget->addOption("When selected");
				activeWhenWidget->addOption("Always");
				activeWhenWidget->entangle(this->activewhen);
				inspector->add(activeWhenWidget);
				
				inspector->add(Widgets::EditableValue<int>::make(this->blurSize));
				
				inspector->add(this->highValue);
				inspector->add(this->lowValue);
			}
			
			//----------
			void Focus::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->activewhen, json);
			}
			
			//----------
			void Focus::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->activewhen, json);
			}
			
			//----------
			ofxCvGui::PanelPtr Focus::getView() {
				return this->view;
			}
			
			//----------
			bool Focus::getRunFinderEnabled() const {
				auto camera = this->getInput<Item::Camera>();
				if(!camera) {
					return false;
				}
				auto grabber = camera->getGrabber();
				if(!grabber->getIsDeviceOpen()) {
					return false;
				}

				if (this->activewhen == 1) {
					//find when we're set to always find
					return true;
				}
				else if (this->isBeingInspected()) {
					//find when we're selected
					return true;
				}
				else {
					if (camera) {
						auto grabber = camera->getGrabber();
						if (!grabber->getDeviceSpecification().supports(ofxMachineVision::Feature::Feature_FreeRun)) {
							//find when the camera is a single shot camera
							return true;
						}
					}
				}
				
				//if nothing was good, then return false
				return false;
			}
			
			//----------
			void Focus::connect(shared_ptr<ofxMachineVision::Grabber::Simple> grabber) {
				if(grabber) {
					grabber->onNewFrameReceived.addListener([this](ofxMachineVision::FrameEventArgs & frameEvent) {
						this->calculateFocus(frameEvent.frame);
					}, this);
					
					//also perform on existing frame if any
					auto frame = grabber->getFrame();
					if(frame) {
						this->calculateFocus(frame);
					}
				}
			}
			
			//----------
			void Focus::disconnect(shared_ptr<ofxMachineVision::Grabber::Simple> grabber) {
				if(grabber) {
					grabber->onNewFrameReceived.removeListeners(this);
				}
			}
			
			//----------
			void Focus::calculateFocus(shared_ptr<ofxMachineVision::Frame> frame) {
				if(frame) {
					try {
						this->processSettingsMutex.lock();
						auto processSettings = this->processSettings;
						this->processSettingsMutex.unlock();
						
						if(processSettings.enabled) {
							auto & input = frame->getPixels();
							auto width = frame->getPixels().getWidth();
							auto height = frame->getPixels().getHeight();
							
							//--
							//0. Make grayscale
							//--
							//
							//check if we need to make a grayscale version
							auto needsGrayscaleConversion = input.getNumChannels() == 3;
							
							//reallocate if we need to
							if(this->process.width != width || this->process.height != height) {
							 //we reallocate grayscale regardless of if we're going to use it this frame incase the next frame is same dimensions but with a different number of channels
								this->process.grayscale.allocate(width, height, 1);
								this->process.blurred.allocate(width, height, 1);
								this->process.edges.allocate(width, height, 1);
								
								this->process.width = width;
								this->process.height = height;
							}
							
							//convert if we need to
							if(needsGrayscaleConversion) {
								cv::cvtColor(toCv(input), toCv(this->process.grayscale), CV_RGB2GRAY);
							}
							
							auto & grayscale = needsGrayscaleConversion ? this->process.grayscale : input;
							//
							//--
							
							
							//1. Blur
							cv::blur(toCv(grayscale), toCv(this->process.blurred), cv::Size(processSettings.blurSize, processSettings.blurSize));
							
							//2. Take absolute difference between original and blurred
							cv::absdiff(toCv(grayscale), toCv(this->process.blurred), toCv(this->process.edges));
							
							//3. Sum all the pixels in the absolute difference image
							auto total = cv::sum(toCv(this->process.edges))[0];
							
							//4. Take the standard deviation of original image
							cv::Scalar stddev, mean;
							cv::meanStdDev(toCv(grayscale), mean, stddev);
							auto cappedStdDev = max((int) stddev[0], 64); //cap the std deviation at 1/4 dynamic range
							
							//5. Caculate a normalized result value
							auto normalized = total / (double(width * height) * cappedStdDev);
							
							this->resultMutex.lock();
							{
								this->result.highFrequency.getPixels() = this->process.edges;
								this->result.lowFrequency.getPixels() = this->process.blurred;
								this->result.value = normalized;
								this->result.width = this->process.width;
								this->result.height = this->process.height;
								this->result.isFrameNew = true;
							}
							this->resultMutex.unlock();
						}
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}
			
			//----------
			void Focus::updateProcessSettings() {
				this->processSettingsMutex.lock();
				this->processSettings.enabled = this->getRunFinderEnabled();
				this->processSettings.blurSize = this->blurSize;
				this->processSettings.lowValue = this->lowValue;
				this->processSettings.highValue = this->highValue;
				this->processSettingsMutex.unlock();
			}
		}
	}
}
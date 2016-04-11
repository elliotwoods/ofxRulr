#include "pch_RulrNodes.h"
#include "Assets.h"

#include "ofxCvGui/Widgets/Button.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/EditableValue.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Application {
			//----------
			Assets::Assets() {
				RULR_NODE_INIT_LISTENER;
				
				//if the assets refresh, then we should refresh our view
				ofAddListener(ofxAssets::Register::X().evtLoad, this, &Assets::flagRefreshView);
			}
			
			//----------
			Assets::~Assets() {
				ofRemoveListener(ofxAssets::Register::X().evtLoad, this, &Assets::flagRefreshView);
			}
			
			//----------
			string Assets::getTypeName() const {
				return "Application::Assets";
			}
			
			//----------
			void Assets::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->view = make_shared<Panels::Widgets>();
				
				this->filter.setName("Filter");
			}

			//----------
			void Assets::update() {
				if(this->viewDirty) {
					this->refreshView();
				}
			}
			
			//----------
			void Assets::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				
				inspector->add(new Widgets::Button("Reload assets", [this]() {
					ofxAssets::Register::X().refresh();
				}));
			}
			
			//----------
			void Assets::serialize(Json::Value & json) {
				json << this->filter;
			}
			
			//----------
			void Assets::deserialize(const Json::Value & json) {
				json >> this->filter;
			}
			
			//----------
			ofxCvGui::PanelPtr Assets::getPanel() {
				return this->view;
			}
			
			//----------
			void Assets::flagRefreshView() {
				this->viewDirty = true;
			}
			
			//----------
			void Assets::refreshView() {
				auto filterString = ofToLower(this->filter.get());
				this->view->clear();
				this->ownedElements.clear();
				
				auto filterWidget = make_shared<Widgets::EditableValue<string>>(this->filter);
				auto filterWidgetWeak = weak_ptr<Element>(filterWidget);
				filterWidget->onValueChange += [this, filterWidgetWeak](const string &) {
					auto filterWidget = filterWidgetWeak.lock();
					this->refreshView();
				};
				this->view->add(filterWidget);

				auto addReloadButton = [this](ElementPtr element, shared_ptr<ofxAssets::BaseAsset> asset) {
					auto button = make_shared<Widgets::Button>("Reload",
															[asset]() {
																asset->reload();
															});
					this->ownedElements.push_back(button);
					button->addListenersToParent(element);
					element->onBoundsChange += [button](BoundsChangeArguments & args) {
						auto bounds = args.localBounds;
						const int width = 80;
						bounds.width = width;
						bounds.x = args.localBounds.width - width;
						button->setBounds(bounds);
					};
				};
				
				auto & assetRegister = ofxAssets::Register::X();
				
				this->view->addTitle("Images");
				{
					const auto & images = assetRegister.getImages();
					const auto & names = images.getNames();
					for(const auto & name : names) {
						if (!filterString.empty()) {
							if(ofToLower(name).find(filterString) == string::npos) {
								continue;
							}
						}
						
						auto asset = images[name];
						auto & image = asset->get();
						auto width = image.getWidth();
						auto height = image.getHeight();
						
						auto element = make_shared<Element>();
						
						float longestAxis = max(width, height);
						auto scaleFactor = 48.0f / longestAxis;
						auto drawWidth = width * scaleFactor;
						auto drawHeight = height * scaleFactor;
						
						element->onDraw += [asset, name, width, height, drawWidth, drawHeight](DrawArguments & args) {
							stringstream text;
							text << name << endl;
							text << width << "x" << height;
							ofxCvGui::Utils::drawText(text.str(), 58, 0);
							
							asset->get().draw(5, 0, drawWidth, drawHeight);
						};
						element->setHeight(58.0f);
						this->view->add(element);
						
						addReloadButton(element, asset);
					}
				}
				
				this->view->addTitle("Shaders");
				{
					const auto & shaders = assetRegister.getShaders();
					const auto & names = shaders.getNames();
					
					for(const auto & name : names) {
						if (!filterString.empty()) {
							if(ofToLower(name).find(filterString) == string::npos) {
								continue;
							}
						}
						
						auto asset = shaders[name];
						auto & shader = asset->get();
						bool loaded = shader.isLoaded();
						
						auto element = make_shared<Element>();
						
						element->onDraw += [name, loaded](DrawArguments & args) {
							stringstream text;
							text << name << endl;
							text << "Loaded : " << (loaded ? "true" : "false");
							ofxCvGui::Utils::drawText(text.str(), 5, 0);
						};
						element->setHeight(58.0f);
						this->view->add(element);
						
						addReloadButton(element, asset);
					}
				}
				
				this->view->addTitle("Fonts");
				{
					auto fonts = assetRegister.getFonts();
					auto names = fonts.getNames();
					
					for(const auto & name : names) {
						auto sizes = assetRegister.getFontSizes(name);
						
						for(const auto & size : sizes) {
							auto sizeString = ofToString(size);
							if (!filterString.empty()) {
								if(ofToLower(name).find(filterString) == string::npos && filterString != sizeString) {
									continue;
								}
							}
							
							auto asset = fonts[name];
							
							auto element = make_shared<Element>();
							element->onDraw += [name, size, asset](DrawArguments & args) {
								stringstream text;
								text << name << ", size " << size;
								ofxCvGui::Utils::drawText(text.str(), 5, 38);
								
								asset->get(size).drawString("AaBbCcDdEeFf0123456789", 5, 35);
							};
							element->setScissor(true);
							element->setHeight(58.0f);
							this->view->add(element);
							
							addReloadButton(element, asset);
						}
					}
				}
				
				this->view->addTitle("Sounds");
				{
					auto sounds = assetRegister.getSounds();
					auto names = sounds.getNames();
					for(const auto & name : names) {
						if (!filterString.empty()) {
							if(ofToLower(name).find(filterString) == string::npos) {
								continue;
							}
						}
						
						auto asset = sounds[name];
						auto & buffer = asset->getSoundBuffer();
						auto duration = buffer.getDurationMS();
						auto channels = buffer.getNumChannels();
						
						auto element = make_shared<Element>();
						element->onDraw += [name, duration, channels](DrawArguments & args) {
							stringstream text;
							text << name << endl;
							text << "Duration = " << duration << ". Channels = " << channels;
							
							ofxCvGui::Utils::drawText(text.str(), 0, 5);
						};
						element->setHeight(58.0f);
						this->view->add(element);
						
						auto playButton = make_shared<Widgets::Toggle>("Play",
						[asset]() {
							return asset->getSoundPlayer().isPlaying();
						},
						[asset](bool play) {
							if(play) {
								asset->getSoundPlayer().play();
							}
						});
						this->ownedElements.push_back(playButton);
						playButton->addListenersToParent(element);
						
						element->onBoundsChange += [playButton](BoundsChangeArguments & args) {
							auto bounds = args.localBounds;
							bounds.width = 64;
							bounds.x = args.localBounds.width - (80 + 64);
							playButton->setBounds(bounds);
						};
						
						addReloadButton(element, asset);
					}
				}
				this->viewDirty = false;
			}
			
		}
	}
}
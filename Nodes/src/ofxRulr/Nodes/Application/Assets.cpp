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
			}
			
			//----------
			string Assets::getTypeName() const {
				return "Application::Assets";
			}
			
			//----------
			void Assets::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				
				this->view = make_shared<Panels::Widgets>();
				
				//if the assets refresh, then we should refresh our view
				ofAddListener(ofxAssets::Register::X().evtLoad, this, &Assets::refreshView);
				
				this->filter.setName("Filter");
				this->refreshView();
			}
			
			//----------
			void Assets::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				
				inspector->add(Widgets::Button::make("Reload assets", [this]() {
					ofxAssets::Register::X().refresh();
				}));
			}
			
			//----------
			void Assets::serialize(Json::Value & json) {
				ofxRulr::Utils::Serializable::serialize(this->filter, json);
			}
			
			//----------
			void Assets::deserialize(const Json::Value & json) {
				ofxRulr::Utils::Serializable::deserialize(this->filter, json);
			}
			
			//----------
			ofxCvGui::PanelPtr Assets::getView() {
				return this->view;
			}
			
			//----------
			void Assets::refreshView() {
				auto filterString = ofToLower(this->filter.get());
				this->view->clear();
				this->ownedElements.clear();
				
				auto filterWidget = Widgets::EditableValue<string>::make(this->filter);
				auto filterWidgetWeak = weak_ptr<Element>(filterWidget);
				filterWidget->onValueChange += [this, filterWidgetWeak](const string &) {
					auto filterWidget = filterWidgetWeak.lock();
					this->refreshView();
				};
				this->view->add(filterWidget);

				auto & assetRegister = ofxAssets::Register::X();
				
				this->view->add("Images");
				{
					auto imageNames = assetRegister.getImageNames();
					for(const auto & imageName : imageNames) {
						if (!filterString.empty()) {
							if(ofToLower(imageName).find(filterString) == string::npos) {
								continue;
							}
						}
						
						auto asset = assetRegister.getImagePointer(imageName);
						auto element = make_shared<Element>();
						
						float longestAxis = max(asset->getWidth(), asset->getHeight());
						auto scaleFactor = 48.0f / longestAxis;
						auto drawWidth = asset->getWidth() * scaleFactor;
						auto drawHeight = asset->getHeight() * scaleFactor;
						
						element->onDraw += [asset, imageName, drawWidth, drawHeight](DrawArguments & args) {
							stringstream text;
							text << imageName << endl;
							text << asset->getWidth() << "x" << asset->getHeight();
							ofxCvGui::Utils::drawText(text.str(), 58, 0);
							
							asset->draw(5, 0, drawWidth, drawHeight);
						};
						element->setHeight(58.0f);
						this->view->add(element);
					}
				}
				
				this->view->add("Shaders");
				{
					auto shaderNames = assetRegister.getShaderNames();
					for(const auto & shaderName : shaderNames) {
						if (!filterString.empty()) {
							if(ofToLower(shaderName).find(filterString) == string::npos) {
								continue;
							}
						}
						
						auto asset = assetRegister.getShaderPointer(shaderName);
						auto element = make_shared<Element>();
						
						bool loaded = asset->isLoaded();
						element->onDraw += [shaderName, asset, loaded](DrawArguments & args) {
							stringstream text;
							text << shaderName << endl;
							text << "Loaded : " << (loaded ? "true" : "false");
							ofxCvGui::Utils::drawText(text.str(), 5, 0);
						};
						element->setHeight(58.0f);
						this->view->add(element);
					}
				}
				
				this->view->add("Fonts");
				{
					auto fontFileNames = assetRegister.getFontFilenames();
					for(const auto & fontFileName : fontFileNames) {
						auto fontSizes = assetRegister.getFontSizes(fontFileName);
						
						for(const auto & fontSize : fontSizes) {
							auto fontSizeString = ofToString(fontSize);
							if (!filterString.empty()) {
								if(ofToLower(fontFileName).find(filterString) == string::npos && filterString != fontSizeString) {
									continue;
								}
							}
							
							auto asset = assetRegister.getFontPointer(fontFileName, fontSize);
							
							auto element = make_shared<Element>();
							element->onDraw += [fontFileName, fontSize, asset](DrawArguments & args) {
								stringstream text;
								text << fontFileName << ", size " << fontSize;
								ofxCvGui::Utils::drawText(text.str(), 5, 38);
								
								asset->drawString("AaBbCcDdEeFf0123456789", 5, 35);
							};
							element->setScissor(true);
							element->setHeight(58.0f);
							this->view->add(element);
						}
					}
				}
				
				this->view->add("Sounds");
				{
					auto soundNames = assetRegister.getSoundNames();
					for(const auto & soundName : soundNames) {
						if (!filterString.empty()) {
							if(ofToLower(soundName).find(filterString) == string::npos) {
								continue;
							}
						}
						
						auto asset = assetRegister.getSoundPointer(soundName);
						auto duration = asset->buffer.getDurationMS();
						auto channels = asset->buffer.getNumChannels();
						
						auto element = make_shared<Element>();
						element->onDraw += [soundName, asset, duration, channels](DrawArguments & args) {
							stringstream text;
							text << soundName << endl;
							text << "Duration = " << duration << ". Channels = " << channels;
							
							ofxCvGui::Utils::drawText(text.str(), 0, 5);
						};
						element->setHeight(58.0f);
						this->view->add(element);
						
						auto playButton = Widgets::Toggle::make("Play", [asset]() {
							return asset->player.isPlaying();
						}, [asset](bool play) {
							if(play) {
								asset->player.play();
							}
						});
						this->ownedElements.push_back(playButton);
						playButton->addListenersToParent(element);
						
						element->onBoundsChange += [playButton](BoundsChangeArguments & args) {
							auto bounds = args.localBounds;
							bounds.width = 64;
							bounds.x = args.localBounds.width - bounds.width;
							playButton->setBounds(bounds);
						};
					}
				}
			}
		}
	}
}
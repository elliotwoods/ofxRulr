#include "pch_RulrNodes.h"
#include "Database.h"
#include "Generator/Base.h"

#include "ofxCvGui/Widgets/Button.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Data {
			namespace Channels {
				//----------
				Database::Database() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Database::getTypeName() const {
					return "Data::Channels::Database";
				}

				//----------
				void Database::init() {
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;

					this->rootChannel = make_shared<Channel>("/");
					this->rootChannel->onHeirarchyChange += [this]() {
						this->needsRebuild = true;
					};

					this->treeView = make_shared<Panels::Tree>();
					this->detailView = make_shared<Panels::Widgets>();
					
					//grid view
					this->view = make_shared<Panels::Groups::Strip>(Panels::Groups::Strip::Direction::Vertical);
					this->view->setCellSizes({ -1, 180 });
					this->view->add(this->treeView);
					this->view->add(this->detailView);

					auto & root = *this->rootChannel;
				}

				//----------
				void Database::update() {
					if (this->needsRebuild) {
						this->rebuildTree();
						this->needsRebuild = false;
					}
					for (auto it = this->generators.begin(); it != this->generators.end(); ) {
						if (it->expired()) {
							it = this->generators.erase(it);
							continue;
						}
						else {
							auto generator = it->lock();
							const auto address = generator->getAddress();

							auto & channel = (*this->rootChannel)[address];
							generator->populateData(channel);

							it++;
						}
					}
				}

				//----------
				void Database::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;

					inspector->add(Widgets::Button::make("Clear", [this]() {
						this->clear();
					}));
				}

				//----------
				PanelPtr Database::getView() {
					return this->view;
				}

				//----------
				shared_ptr<Channel> Database::getRootChannel() {
					return this->rootChannel;
				}

				//----------
				void Database::clear() {
					this->rootChannel->clear();
				}

				//----------
				void Database::addGenerator(shared_ptr<Nodes::Data::Channels::Generator::Base> provider) {
					this->generators.push_back(provider);
				}

				//----------
				void Database::removeGenerator(Nodes::Data::Channels::Generator::Base * provider) {
					for (auto it = this->generators.begin(); it != this->generators.end(); it++) {
						auto haystack = it->lock();
						if (haystack.get() == provider) {
							this->generators.erase(it);
							break;
						}
					}
				}

				//----------
				void Database::rebuildTree() {
					auto rootBranch = this->treeView->getRootBranch();
					rootBranch->clear();
					rootBranch->setCaption(this->rootChannel->getName());
					this->addGenerationToGui(rootBranch, *this->getRootChannel());
				}

				//----------
				void Database::rebuildDetailView() {
					this->detailView->clear();

					auto selectedChannel = this->selectedChannel.lock();
					if (selectedChannel) {
						this->detailView->add(selectedChannel->getName());
						selectedChannel->getParameterUntyped();
						auto type = selectedChannel->getValueType();
						switch (type) {
						case Channel::Type::Int:
						{
							const auto & parameter = selectedChannel->getParameter<int>();
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "int"; }));
							this->detailView->add(Widgets::EditableValue<int>::make(*parameter));
							break;
						}
							
						case Channel::Type::Float:
						{
							const auto & parameter = selectedChannel->getParameter<float>();
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "string"; }));
							this->detailView->add(Widgets::EditableValue<float>::make(*parameter));
							break;
						}

						case Channel::Type::String:
						{
							const auto & parameter = selectedChannel->getParameter<string>();
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "string"; }));
							this->detailView->add(Widgets::EditableValue<string>::make(*parameter));
							break;
						}

						case Channel::Type::Vec3f:
						{
							const auto & parameter = selectedChannel->getParameter<ofVec3f>();
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "Vec3f"; }));
							this->detailView->add(Widgets::EditableValue<ofVec3f>::make(*parameter));
							break;
						}

						case Channel::Type::Vec4f:
						{
							const auto & parameter = selectedChannel->getParameter<ofVec4f>();
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "ofVec4f"; }));
							this->detailView->add(Widgets::EditableValue<ofVec4f>::make(*parameter));
							break;
						}
						case Channel::Type::Undefined:
						{
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "Undefined"; }));
							break;
						}
						default:
						{
							this->detailView->add(Widgets::LiveValue<string>::make("Type", []() {return "Unknown"; }));
							break;
						}
						}
					}
				}

				//----------
				void Database::addGenerationToGui(shared_ptr<Panels::Tree::Branch> guiBranch, const Channel & channel) {
					const auto & subChannels = channel.getSubChannels();
					for (const auto & subChannel : subChannels) {
						auto subBranch = make_shared<Panels::Tree::Branch>();
						subBranch->setCaption(subChannel.second->getName());
						this->addGenerationToGui(subBranch, *subChannel.second);
						guiBranch->addBranch(subBranch);

						weak_ptr<Channel> weakChannel = subChannel.second;
						subBranch->onMouseReleased += [this, weakChannel](MouseArguments & args) {
							this->selectChannel(weakChannel);
						};

						subBranch->onDraw.addListener([this, weakChannel](DrawArguments & args) {
							auto selectedChannel = this->selectedChannel.lock();
							auto thisChannel = weakChannel.lock();
							auto isSelected = thisChannel == selectedChannel;
							ofPushStyle();
							{
								ofFill();
								ofSetLineWidth(0);
								ofSetColor(isSelected ? 100 : 50, 100);
								ofDrawRectangle(args.localBounds);
							}
							ofPopStyle();
						}, -100, this);
					}
				}

				//----------
				void Database::selectChannel(weak_ptr<Channel> channel) {
					this->selectedChannel = channel;
					this->rebuildDetailView();
				}
			}
		}
	}
}
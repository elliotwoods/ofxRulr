#include "Transmit.h"
#include "ofxCvGui/Panels/Scroll.h"
#include "ofxCvGui/Widgets/Title.h"
#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Utils/Utils.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace DMX {
#pragma mark Transmit::Universe
			//----------
			Transmit::Universe::Universe() {
				this->clearChannels();
				this->previewDirty = true;
				this->preview.allocate(32, 16, GL_LUMINANCE);
				this->preview.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
				this->blackoutEnabled.set("Blackout enabled", false);
			}

			//----------
			void Transmit::Universe::setChannel(ChannelIndex channel, Value value) {
				if (channel > 512) {
					RULR_ERROR << "ofxRulr::Nodes::DMX::Transmit : Channel index " << (int)(channel) << " is invalid";
				}
				else {
					this->values[channel] = value;
					this->previewDirty = true;
				}
			}

			//----------
			void Transmit::Universe::setChannels(ChannelIndex channelOffset, Value * values, ChannelIndex count) {
				if (channelOffset + count > 512) {
					RULR_ERROR << "ofxRulr::Nodes::DMX::Transmit : Channel range " << (int)(channelOffset) << "->" << (int) (channelOffset + count) << " is invalid";
				}
				else {
					memcpy(this->values + channelOffset, values, count);
					this->previewDirty = true;
				}
			}

			//----------
			const Value * Transmit::Universe::getChannels() const {
				return this->values;
			}

			//----------
			const ofTexture & Transmit::Universe::getTextureReference() {
				if (this->previewDirty) {
					this->preview.loadData(this->values, this->preview.getWidth(), this->preview.getHeight(), GL_LUMINANCE);
					this->previewDirty = false;
				}
				return this->preview;
			}

			//----------
			void Transmit::Universe::clearChannels() {
				memset(this->values, 0, 513);
				this->previewDirty = true;
			}

#pragma mark Transmit
			//----------
			Transmit::Transmit() {
				RULR_NODE_INIT_LISTENER;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
			}

			//----------
			void Transmit::init() {
				this->view = make_shared<Panels::Scroll>();
				this->setUniverseCount(1);
				this->firstFrame = true;
			}

			//----------
			void Transmit::update() {
				if (this->firstFrame) {
					//don't send on first frame
					this->firstFrame = false;
				}
				else {
					for (int i = 0; i < this->universes.size(); i++) {
						if (this->universes[i]->blackoutEnabled) {
							this->universes[i]->clearChannels();
						}
						this->sendUniverse(i, this->universes[i]);
					}
				}
			}

			//----------
			string Transmit::getTypeName() const {
				return "DMX::Transmit";
			}

			//----------
			ofxCvGui::PanelPtr Transmit::getView() {
				return this->view;
			}

			//----------
			void Transmit::serialize(Json::Value & json) {
				auto & jsonUniverses = json["universes"];
				for (int i = 0; i < this->universes.size(); i++) {
					auto & jsonUniverse = jsonUniverses[ofToString(i)];
					Utils::Serializable::serialize(this->universes[i]->blackoutEnabled, jsonUniverse);
				}
			}

			//----------
			void Transmit::deserialize(const Json::Value & json) {
				const auto & jsonUniverses = json["universes"];
				for (int i = 0; i < this->universes.size(); i++) {
					const auto & jsonUniverse = jsonUniverses[ofToString(i)];
					Utils::Serializable::deserialize(this->universes[i]->blackoutEnabled, jsonUniverse);
				}
			}

			//----------
			void Transmit::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				for (int i = 0; i < this->universes.size(); i++) {
					inspector->add(Widgets::Title::make("Universe " + ofToString(i)));
					inspector->add(Widgets::Toggle::make(this->universes[i]->blackoutEnabled));
				}
			}

			//----------
			UniverseIndex Transmit::getUniverseCount() const {
				return (UniverseIndex) this->universes.size();
			}

			//----------
			const vector<shared_ptr<Transmit::Universe>> & Transmit::getUniverses() const {
				return this->universes;
			}

			//----------
			shared_ptr<Transmit::Universe> Transmit::getUniverse(UniverseIndex universeIndex) const {
				if (this->universes.size() > universeIndex) {
					return this->universes[universeIndex];
				}
				else {
					RULR_ERROR << "Universe index " << universeIndex << " out of range";
					return shared_ptr<Universe>();
				}
			}

			//----------
			void Transmit::setUniverseCount(UniverseIndex universeCount) {
				if (this->universes.size() > universeCount) {
					this->universes.resize(universeCount);
				}
				else {
					while (universeCount > this->universes.size()) {
						this->universes.push_back(make_shared<Universe>());
					}
				}

				//rebuild gui
				this->view->clear();
				for (int i = 0; i < this->universes.size(); i++) {
					auto universe = universes[i];

					this->view->add(Widgets::Title::make("Universe " + ofToString(i) + ":", Widgets::Title::Level::H3));

					auto preview = make_shared<Element>();
					preview->onDraw += [universe](DrawArguments & args) {
						universe->getTextureReference().draw(args.localBounds);

						if (universe->blackoutEnabled.get()) {
							ofxCvGui::Utils::drawText("BLACKOUT", args.localBounds);
						}
					};
					preview->setBounds(ofRectangle(0, 0, 300, 100));
					this->view->add(preview);
				}
			}
		}
	}
}
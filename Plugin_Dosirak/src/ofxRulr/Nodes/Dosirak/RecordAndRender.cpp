#include "pch_Plugin_Dosirak.h"
#include "RecordAndRender.h"
#include "Curves.h"
#include "ofxRulr/Nodes/AnotherMoon/Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
#pragma mark Frame
			//----------
			void
				RecordAndRender::Frame::serialize(nlohmann::json& json) const
			{
				this->curves.serialize(json["curves"]);

				{
					auto& jsonPictures = json["pictures"];
					for (const auto& it : this->renderedPictures) {
						ofxRulr::Utils::serialize(jsonPictures[ofToString(it.first)], it.second);
					}
				}
			}

			//----------
			void
				RecordAndRender::Frame::deserialize(const nlohmann::json& json)
			{
				if (json.contains("curves")) {
					this->curves.deserialize(json["curves"]);
				}

				if (json.contains("pictures")) {
					this->renderedPictures.clear();

					const auto& jsonPictures = json["pictures"];
					for (auto it = jsonPictures.begin(); it != jsonPictures.end(); it++) {
						auto key = ofToInt(it.key());
						this->renderedPictures.emplace(key, Picture());

						auto& jsonPicture = it.value();
						auto& picture = this->renderedPictures[key];
						ofxRulr::Utils::deserialize(jsonPicture, picture);
					}
				}
			}

			//----------
			shared_ptr<RecordAndRender::Frame>
				RecordAndRender::Frame::clone()
			{
				// this'll work since nothing inside Frames uses pointers for now
				auto frame = make_shared<RecordAndRender::Frame>(*this);
				return frame;
			}

#pragma mark Frames
			//----------
			void
				RecordAndRender::Frames::serialize(nlohmann::json& json) const
			{
				json = nlohmann::json::array();

				for (const auto& frame : *this) {
					nlohmann::json jsonFrame;
					frame->serialize(jsonFrame);
					json.push_back(jsonFrame);
				}
			}

			//----------
			void
				RecordAndRender::Frames::deserialize(const nlohmann::json& json)
			{
				this->clear();
				for (const auto& jsonFrame : json) {
					auto frame = make_shared<Frame>();
					frame->deserialize(jsonFrame);
					this->push_back(frame);
				}
			}

			//----------
			RecordAndRender::Frames
				RecordAndRender::Frames::clone()
			{
				Frames frames;
				for (auto& frame : *this) {
					frames.push_back(frame->clone());
				}
				return frames;
			}

#pragma mark FrameSets
			//----------
			void
				RecordAndRender::FrameSets::serialize(nlohmann::json& json) const
			{
				for (const auto& it : *this) {
					it.second->serialize(json[it.first]);
				}
			}

			//----------
			void
				RecordAndRender::FrameSets::deserialize(const nlohmann::json& json)
			{
				this->clear();
				for (auto it = json.begin(); it != json.end(); it++) {
					auto frames = make_shared<Frames>();
					frames->deserialize(it.value());
					this->emplace(it.key(), frames);
				}
			}

#pragma mark RecordAndRender
			//----------
			RecordAndRender::RecordAndRender()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				RecordAndRender::getTypeName() const
			{
				return "Dosirak::RecordAndRender";
			}

			//----------
			void
				RecordAndRender::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				{
					auto curvesInput = this->addInput<Curves>();

					curvesInput->onNewConnection += [this](shared_ptr<Curves> curvesNode) {
						curvesNode->onNewCurves.addListener([this]() {
							auto curvesNode = this->getInput<Curves>();
							if (curvesNode) {
								this->callbackNewCurves(curvesNode->getCurvesRaw());
							}
							}, this);
					};

					curvesInput->onDeleteConnection += [this](shared_ptr<Curves> curvesNode) {
						if (curvesNode) {
							curvesNode->onNewCurves.removeListeners(this);
						}
					};
				}

				this->addInput<AnotherMoon::Lasers>();

				this->manageParameters(this->parameters);

				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					this->panel = panel;
					this->refreshPanel();
				}
			}

			//----------
			void
				RecordAndRender::update()
			{
				if (this->playPosition > this->frames.size()) {
					this->playPosition = 0;
				}

				if (this->state.get() == State::Play) {
					this->play();
				}
			}

			//----------
			ofxCvGui::PanelPtr
				RecordAndRender::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				RecordAndRender::populateInspector(ofxCvGui::InspectArguments& args)
			{

			}

			//----------
			void
				RecordAndRender::serialize(nlohmann::json& json) const
			{
				this->frameSets.serialize(json["frameSets"]);
			}

			//----------
			void
				RecordAndRender::deserialize(const nlohmann::json& json)
			{
				if (json.contains("frameSets")) {
					this->frameSets.deserialize(json["frameSets"]);
				}
				this->refreshPanel();
			}

			//----------
			void
				RecordAndRender::recordCurves(const Data::Dosirak::Curves& curves)
			{
				// Check if it's a duplicate
				if (this->parameters.recording.ignoreDuplicates) {
					if (!this->frames.empty()) {
						auto lastFrameCurves = this->frames.back()->curves;
						if (!(lastFrameCurves != curves)) {
							return;
						}
					}
				}

				// Otherwise add it (don't render for now)
				{
					auto frame = make_shared<Frame>();
					frame->curves = curves;
					this->frames.push_back(frame);
				}

				// Set the play position to point to the last frame
				this->playPosition = this->frames.size() - 1;
			}

			//----------
			void
				RecordAndRender::render()
			{
				this->throwIfMissingAConnection<AnotherMoon::Lasers>();
				this->throwIfMissingAConnection<Curves>();

				auto lasersNode = this->getInput<AnotherMoon::Lasers>();
				auto lasers = lasersNode->getLasersSelected();

				auto curvesNode = this->getInput<Curves>();
				
				const auto useFastMethod = this->parameters.rendering.useFastMethod.get();

				Utils::ScopedProcess scopedProcess("Render frames", true, this->frames.size());
				
				map<int, vector<glm::vec2>> priorPictures; // for speeding up rendering

				for (auto frame : this->frames) {
					Utils::ScopedProcess scopedProcessFrame("Frame", true);

					// Render the pictures
					for (auto laser : lasers) {
						auto serialNumber = laser->parameters.serialNumber.get();

						// Check if it's already rendered
						if (frame->renderedPictures.find(serialNumber) != frame->renderedPictures.end()) {
							// Go to next laser
							continue;
						}

						auto worldPoints = curvesNode->getWorldPoints(frame->curves);
						
						vector<glm::vec2> priorPicture;
						{
							auto findPriorPicture = priorPictures.find(serialNumber);
							if (findPriorPicture != priorPictures.end()) {
								priorPicture = findPriorPicture->second;
							}
						}

						
						auto projectorPoints = useFastMethod
							? laser->renderWorldPointsFast(worldPoints)
							: laser->renderWorldPoints(worldPoints, priorPicture);
						
						// Prune projector points outside range
						if (this->parameters.rendering.prunePointsOutsideRange.get()) {
							Picture prunedPicture;
							ofRectangle limits(-1, -1, 2, 2);
							
							for (auto& projectorPoint : projectorPoints) {
								if (limits.inside(projectorPoint)) {
									prunedPicture.push_back(projectorPoint);
								}
							}
							projectorPoints = prunedPicture;
						}
						
						try {
							laser->drawPicture(projectorPoints);
						}
						RULR_CATCH_ALL_TO_ERROR;

						frame->renderedPictures.emplace(serialNumber, projectorPoints);
					}

					// Draw the pictures
					if (this->parameters.rendering.drawPictures.get()) {
						for (auto laser : lasers) {
							auto serialNumber = laser->parameters.serialNumber.get();

							auto it = frame->renderedPictures.find(serialNumber);
							laser->drawPicture(it->second);
						}
					}
					scopedProcessFrame.end();
				}
				scopedProcess.end();

				if (this->parameters.rendering.playAfterRender) {
					this->state.set(State::Play);
				}
			}

			//----------
			void
				RecordAndRender::play()
			{
				if (this->playPosition > this->frames.size()) {
					this->playPosition = -1;
				}

				if (this->frames.empty()) {
					return;
				}

				this->playPosition++;

				presentCurrentFrame();
			}

			//----------
			void
				RecordAndRender::clear()
			{
				this->frames.clear();
			}

			//----------
			void
				RecordAndRender::clearRenders()
			{
				for (auto frame : this->frames) {
					frame->renderedPictures.clear();
				}
			}

			//----------
			void
				RecordAndRender::store(string name)
			{
				if (name == "") {
					name = ofSystemTextBoxDialog("Recording name");
				}

				this->frameSets[name] = make_shared<Frames>(this->frames.clone());
				this->refreshPanel();
			}

			//----------
			void
				RecordAndRender::recall(string name)
			{
				if (name == "") {
					name = ofSystemTextBoxDialog("Recording name");
				}

				auto it = this->frameSets.find(name);
				if (it == this->frameSets.end()) {
					throw(ofxRulr::Exception("Can't find frameSet " + name));
				}

				// Deep copy the frames (in case we edit and store as different copy)
				this->frames = it->second->clone();
				presentCurrentFrame();
			}

			//----------
			void
				RecordAndRender::clearFrameSets()
			{
				this->frameSets.clear();
				this->refreshPanel();
			}

			//----------
			void
				RecordAndRender::transportGotoFirst()
			{
				this->playPosition.set(0);
				presentCurrentFrame();
			}

			//----------
			void
				RecordAndRender::transportGotoLast()
			{
				this->playPosition.set(this->frames.size() - 1);
				presentCurrentFrame();
			}

			//----------
			void
				RecordAndRender::transportGotoNext()
			{
				auto position = this->playPosition.get();
				position++;
				if (position < this->frames.size()) {
					this->playPosition.set(position);
				}

				presentCurrentFrame();
			}

			//----------
			void
				RecordAndRender::transportGotoPrevious()
			{
				auto position = this->playPosition.get();
				position--;
				if (position >= 0) {
					this->playPosition.set(position);
				}

				presentCurrentFrame();
			}

			//----------
			void
				RecordAndRender::presentCurrentFrame()
			{
				// Double check frame index is OK
				if (this->playPosition >= this->frames.size()
					|| this->playPosition < 0) {
					return;
				}

				auto frame = this->frames[this->playPosition];

				// Send to lasers
				{
					auto lasersNode = this->getInput<AnotherMoon::Lasers>();
					if (lasersNode) {
						auto lasers = lasersNode->getLasersSelected();
						for (auto laser : lasers) {
							auto serialNumber = laser->parameters.serialNumber.get();

							// find the rendered frame
							auto it = frame->renderedPictures.find(serialNumber);
							if (it != frame->renderedPictures.end()) {
								auto& projectorPoints = it->second;

								try {
									laser->drawPicture(projectorPoints);
								}
								RULR_CATCH_ALL_TO_ERROR;
							}
						}
					}
				}

				// Send to curves node
				auto curvesNode = this->getInput<Curves>();
				if (curvesNode) {
					curvesNode->setCurves(frame->curves);
				}
			}

			//----------
			vector<string>
				RecordAndRender::getStoredRecordingNames() const
			{
				vector<string> frameSetNames;
				frameSetNames.reserve(this->frameSets.size());
				for (const auto& it : this->frameSets) {
					frameSetNames.push_back(it.first);
				}
				return frameSetNames;
			}

			//----------
			void
				RecordAndRender::refreshPanel()
			{
				this->panel->clear();

				// Record controls
				{
					auto strip = panel->addHorizontalStack();

					// Record button
					{
						auto toggle = make_shared<ofxCvGui::Widgets::Toggle>("Record", [this]() {
							return this->state.get() == State::Record;
							}
							, [this](bool value) {
								if (value) {
									this->state.set(State::Record);
								}
								else {
									this->state.set(State::Pause);
								}
							});
						toggle->setDrawGlyph(u8"\uf111");
						strip->add(toggle);
					}

					// Play button
					{
						auto toggle = make_shared<ofxCvGui::Widgets::Toggle>("Play", [this]() {
							return this->state.get() == State::Play;
							}
							, [this](bool value) {
								if (value) {
									this->state.set(State::Play);
								}
								else {
									this->state.set(State::Pause);
								}
							});
						toggle->setDrawGlyph(u8"\uf04b");
						strip->add(toggle);
					}

					// Pause button
					{
						auto toggle = make_shared<ofxCvGui::Widgets::Toggle>("Pause", [this]() {
							return this->state.get() == State::Pause;
							}
							, [this](bool value) {
								if (value) {
									this->state.set(State::Pause);
								}
								else {
									this->state.set(State::Pause);
								}
							});
						toggle->setDrawGlyph(u8"\uf04c");
						strip->add(toggle);
					}

					// Clear renders button
					{
						auto button = make_shared<ofxCvGui::Widgets::Button>("Clear renders", [this]() {
							this->clearRenders();
							});
						button->setDrawGlyph(u8"\uf12d"); // Eraser
						strip->add(button);
					}

					// Clear button
					{
						auto button = make_shared<ofxCvGui::Widgets::Button>("Delete frames", [this]() {
							this->clear();
							});
						button->setDrawGlyph(u8"\uf1f8"); // Trash
						strip->add(button);
					}
				}

				// Play position
				{
					// Numbers
					{
						auto strip = panel->addHorizontalStack();
						strip->add(make_shared<ofxCvGui::Widgets::LiveValue<size_t>>("Frame count", [this]() {
							return this->frames.size();
							}));
						strip->add(make_shared<ofxCvGui::Widgets::EditableValue<int>>(this->playPosition));
					}

					// Transport controls
					{
						auto strip = panel->addHorizontalStack();
						strip->addButton("First", [this]() { this->transportGotoFirst(); })->setDrawGlyph(u8"\uf049");
						strip->addButton("Previous", [this]() { this->transportGotoPrevious(); })->setDrawGlyph(u8"\uf048");
						strip->addButton("Next", [this]() { this->transportGotoNext(); })->setDrawGlyph(u8"\uf051");
						strip->addButton("Last", [this]() { this->transportGotoLast(); })->setDrawGlyph(u8"\uf050");
					}
				}

				// Render button
				{
					auto button = panel->addButton("Render", [this]() {
						try {
							this->render();
						}
						RULR_CATCH_ALL_TO_ERROR;
						});
				}

				panel->addSpacer();

				// Recordings
				{
					{
						auto strip = panel->addHorizontalStack();
						strip->addButton("Store", [this]() {
							try {
								this->store();
							}
							RULR_CATCH_ALL_TO_ALERT;
							})->setDrawGlyph(u8"\uf019");
						strip->addButton("Load", [this]() {
							try {
								this->recall();
							}
							RULR_CATCH_ALL_TO_ALERT;
							})->setDrawGlyph(u8"\uf093");
						strip->addButton("Clear", [this]() {
							try {
								this->clearFrameSets();
							}
							RULR_CATCH_ALL_TO_ALERT;
							})->setDrawGlyph(u8"\uf1f8");
					}

					auto recordingNames = this->getStoredRecordingNames();
					for (const auto& recordingName : recordingNames) {
						auto button = panel->addButton(recordingName, [this, recordingName]() {
							try {
								this->recall(recordingName);
							}
							RULR_CATCH_ALL_TO_ALERT;
							});
					}
				}
			}

			//----------
			void
				RecordAndRender::callbackNewCurves(const Data::Dosirak::Curves& curves)
			{
				if (this->state.get() == State::Record) {
					this->recordCurves(curves);
				}
			}
		}
	}
}
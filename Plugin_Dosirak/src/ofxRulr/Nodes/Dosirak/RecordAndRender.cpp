#include "pch_Plugin_Dosirak.h"
#include "RecordAndRender.h"
#include "Curves.h"
#include "ofxRulr/Nodes/AnotherMoon/Lasers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Dosirak {
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

					// Record button
					{
						auto toggle = panel->addToggle("Record", [this]() {
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
					}

					// Play button
					{
						auto toggle = panel->addToggle("Play", [this]() {
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
					}

					// Pause button
					{
						auto toggle = panel->addToggle("Pause", [this]() {
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
					}

					// Clear button
					{
						auto button = panel->addButton("Clear", [this]() {
							this->clear();
							});
						button->setDrawGlyph(u8"\uf1f8");
					}

					panel->addLiveValue<size_t>("Frame count", [this]() {
						return this->frames.size();
						});
					panel->addEditableValue<int>(this->playPosition);

					// Render button
					{
						auto button = panel->addButton("Render", [this]() {
							try {
								this->render();
							}
							RULR_CATCH_ALL_TO_ERROR;
							});
						button->setHeight(100.0f);
					}

					this->panel = panel;
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

			}

			//----------
			void
				RecordAndRender::deserialize(const nlohmann::json& json)
			{

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

				Utils::ScopedProcess scopedProcess("Render frames", true, this->frames.size());
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
						auto projectorPoints = laser->renderWorldPoints(worldPoints);

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

				// Double check
				if (this->playPosition >= this->frames.size()) {
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
								laser->drawPicture(it->second);
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
			void
				RecordAndRender::clear()
			{
				this->frames.clear();
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
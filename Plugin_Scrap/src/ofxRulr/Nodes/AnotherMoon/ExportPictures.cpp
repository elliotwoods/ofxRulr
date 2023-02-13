#include "pch_Plugin_Scrap.h"
#include "ExportPictures.h"

#include "DrawMoon.h"
#include "Lasers.h"
#include "Moon.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			ExportPictures::ExportPictures()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				ExportPictures::getTypeName() const
			{
				return "AnotherMoon::ExportPictures";
			}

			//----------
			void
				ExportPictures::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<Moon>();
				this->addInput<Lasers>();
				this->addInput<DrawMoon>();
				this->manageParameters(this->parameters);
			}

			//----------
			void
				ExportPictures::update()
			{
				if (this->parameters.preview.enabled) {
					// Update position in animation
					auto position = this->parameters.preview.position.get();
					position += ofGetLastFrameTime() / this->parameters.timeline.duration.get();
					if (position > 1.0f) {
						position -= floor(position);
					}
					this->parameters.preview.position.set(position);

					try {
						this->drawFrame(position);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			void
				ExportPictures::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				
				inspector->addTitle("Keyframes", ofxCvGui::Widgets::Title::Level::H2);
				{
					size_t index = 0;
					for (auto keyframe : this->keyframes) {
						inspector->addParameterGroup(*keyframe);
						inspector->addButton("Remove", [this, index]() {
							this->removeKeyframe(index);
							});
						index++;
					}
				}

				inspector->addButton("Add keyframe", [this]() {
					this->addKeyframe();
					});

				inspector->addSpacer();

				inspector->addButton("Export pictures...", [this]() {
					try {
						auto result = ofSystemLoadDialog("Select output folder", true);
						if (result.bSuccess) {
							this->exportPictures(std::filesystem::path(result.filePath));
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			void
				ExportPictures::serialize(nlohmann::json& json) const
			{
				auto & jsonKeyFrames = json["keyFrames"];
				for (auto keyframe : this->keyframes) {
					nlohmann::json jsonKeyFrame;
					Utils::serialize(jsonKeyFrame, *keyframe);
					jsonKeyFrames.push_back(jsonKeyFrame);
				}
			}

			//----------
			void
				ExportPictures::deserialize(const nlohmann::json& json)
			{
				this->keyframes.clear();
				if (json.contains("keyFrames")) {
					const auto& jsonKeyFrames = json["keyFrames"];
					size_t index = 0;
					for (const auto& jsonKeyFrame : jsonKeyFrames) {
						auto keyframe = make_shared<Frame>();
						keyframe->setName(ofToString(index));

						Utils::deserialize(jsonKeyFrame, *keyframe);
						this->keyframes.push_back(keyframe);

						index++;
					}
				}

				this->updateKeyframes();
			}

			//----------
			void
				ExportPictures::exportPictures(std::filesystem::path& outputFolder) const
			{
				this->throwIfMissingAnyConnection();
				auto lasersNode = this->getInput<Lasers>();
				auto lasers = lasersNode->getLasersSelected();

				map<size_t, nlohmann::json> dataPerLaser; // by positionIndex

				// setup general parameters
				{
					nlohmann::json infoJson;
					infoJson["duration"] = this->parameters.timeline.duration.get();
					infoJson["frames"] = this->parameters.timeline.frames.get();

					// push info into the data
					for (auto laser : lasers) {
						auto laserPositionIndex = laser->parameters.positionIndex.get();
						dataPerLaser[laserPositionIndex]["info"] = infoJson;
						dataPerLaser[laserPositionIndex]["frames"] = nlohmann::json::array();
					}
				}

				auto frameCount = this->parameters.timeline.frames.get();

				{
					Utils::ScopedProcess scopedProcessFrames("Rendering frames", true, frameCount);
					// Render the frames
					for (size_t i = 0; i < frameCount; i++) {
						Utils::ScopedProcess scopedProcessFrame("Frame #" + ofToString(i));

						auto position = (float)i / (float)frameCount;
						auto frame = this->renderFrame(position);
						this->drawFrame(frame);
						for (auto laser : lasers) {
							nlohmann::json frameJson;
							auto positionIndex = laser->parameters.positionIndex.get();

							// picture
							{
								const auto& picture = laser->getLastPicture();
								vector<float> values;
								for (const auto& point : picture) {
									values.push_back(point.x);
									values.push_back(point.y);
								}

								frameJson["picture"] = values;
							}

							// color
							{
								frameJson["color"]["red"] = frame.brightness.get();
								frameJson["color"]["green"] = frame.brightness.get();
								frameJson["color"]["blue"] = frame.brightness.get();
							}

							// save to frames
							dataPerLaser[positionIndex]["frames"].push_back(frameJson);
						}
						scopedProcessFrame.end();
					}
					scopedProcessFrames.end();
				}

				// Save the outputs
				{
					Utils::ScopedProcess scopedProcess("Saving files");
					for (auto laser : lasers) {
						auto positionIndex = laser->parameters.positionIndex.get();
						auto path = outputFolder / (ofToString(positionIndex) + ".json");

						ofstream file(path.string(), ios::out);
						{
							file << dataPerLaser[positionIndex].dump(4);
						}
						file.close();
					}
					scopedProcess.end();
				}
			}

			//----------
			void
				ExportPictures::addKeyframe()
			{
				auto keyframe = make_shared<Frame>();

				auto moon = this->getInput<Moon>();
				if (moon) {
					keyframe->position = moon->getPosition();
					keyframe->diameter = moon->getRadius() * 2.0f;
				}

				this->keyframes.push_back(keyframe);
				this->updateKeyframes();
			}

			//----------
			void
				ExportPictures::removeKeyframe(size_t index)
			{
				if (index >= this->keyframes.size()) {
					throw(Exception("removeKeyframe : Keyfarme " + ofToString(index) + " out of range"));
				}

				this->keyframes.erase(this->keyframes.begin() + index);
				this->updateKeyframes();
			}

			//----------
			void
				ExportPictures::updateKeyframes()
			{
				for (size_t i = 0; i < this->keyframes.size(); i++) {
					auto keyframe = this->keyframes[i];
					keyframe->setName(ofToString(i));
				}
				ofxCvGui::InspectController::X().refresh(this);
			}

			//----------
			ExportPictures::Frame
				ExportPictures::renderFrame(float pct) const
			{
				if (this->keyframes.empty()) {
					throw(Exception("No keyframes"));
				}

				// Check if only one keyframe, then no interpolation
				if (this->keyframes.size() == 1) {
					return *this->keyframes.front();
				}

				// Prepare keyframes
				auto keyframes = this->keyframes;
				{
					// Apply the playbackStyle
					switch (this->parameters.timeline.playbackStyle.get()) {
					case PlaybackStyle::Linear:
						// if looping, add end as keyframe also
						if (this->parameters.timeline.loop) {
							keyframes.push_back(keyframes.front());
						}
						break;
					case PlaybackStyle::Sine:
						pct = 1.0f - (cos(pct * TWO_PI) + 1.0f) / 2.0f;
						break;
					default:
						break;
					}
				}
				auto keyframeCount = keyframes.size();
				auto keyframeIndexReal = pct * float(keyframeCount - 1);

				// draw the keyframes
				if (keyframeIndexReal == floor(keyframeIndexReal)) {
					// exactly on a frame
					return *keyframes[(int)keyframeIndexReal];
				}
				else {
					// get adjacent frames
					auto previousFrame = keyframes[floor(keyframeIndexReal)];
					auto nextFrame = keyframes[ceil(keyframeIndexReal)];

					// get ratio
					auto ratio = keyframeIndexReal - floor(keyframeIndexReal);

					// apply interpolation
					Frame interpolatedFrame;
					interpolatedFrame.position.set(previousFrame->position.get() * (1.0f - ratio) + nextFrame->position.get() * ratio);
					interpolatedFrame.diameter.set(previousFrame->diameter.get() * (1.0f - ratio) + nextFrame->diameter.get() * ratio);
					interpolatedFrame.brightness.set(previousFrame->brightness.get() * (1.0f - ratio) + nextFrame->brightness.get() * ratio);

					return interpolatedFrame;
				}
			}
			
			//----------
			void
				ExportPictures::drawFrame(float pct) const
			{
				auto frame = this->renderFrame(pct);
				this->drawFrame(frame);
			}

			//----------
			void
				ExportPictures::drawFrame(const Frame& frame) const
			{
				this->throwIfMissingAnyConnection();

				auto drawMoonNode = this->getInput<DrawMoon>();
				auto moonNode = this->getInput<Moon>();
				auto lasersNode = this->getInput<Lasers>();

				auto selectedLasers = lasersNode->getLasersSelected();

				// Set RGB
				for (auto laser : selectedLasers) {
					laser->parameters.deviceState.projection.color.red.set(frame.brightness.get());
					laser->parameters.deviceState.projection.color.green.set(frame.brightness.get());
					laser->parameters.deviceState.projection.color.blue.set(frame.brightness.get());
					laser->pushColor();
				}

				// Set Geometry
				moonNode->setPosition(frame.position.get());
				moonNode->setDiameter(frame.diameter.get());

				// throw an exception if can't draw
				drawMoonNode->drawLasers(true);
			}
		}
	}
}
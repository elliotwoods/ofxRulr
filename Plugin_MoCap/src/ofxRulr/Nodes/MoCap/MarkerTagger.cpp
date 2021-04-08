#include "pch_Plugin_MoCap.h"
#include "MarkerTagger.h"

#include "ofxRulr/Nodes/Item/Camera.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
#pragma mark Capture
			//----------
			MarkerTagger::Capture::Capture() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			std::string MarkerTagger::Capture::getDisplayString() const {
				stringstream ss;
				ss << "Tag count : " << this->markers.size();
				return ss.str();
			}

			//----------
			void MarkerTagger::Capture::serialize(nlohmann::json & json) {
				{
					auto & jsonMarkers = json["markers"];
					for (const auto & marker : this->markers) {
						nlohmann::json jsonMarker;
						jsonMarker["ID"] = marker.first;
						jsonMarker["x"] = marker.second.x;
						jsonMarker["y"] = marker.second.y;
						jsonMarkers.push_back(jsonMarker);
					}
				}

				{
					ofDirectory::createDirectory("MarkerTagger");
					const auto filename = ofToDataPath("MarkerTagger/" + ofToString(this->timestamp.count()) + ".png");
					json["imageFilename"] = filename;
					cv::imwrite(filename, this->image);
				}

				{
					auto & jsonBoundingBoxes = json["boundingBoxes"];
					for (const auto & boundingBox : this->boundingBoxes) {
						nlohmann::json jsonBoundingBox;
						jsonBoundingBox["x"] = boundingBox.x;
						jsonBoundingBox["y"] = boundingBox.y;
						jsonBoundingBox["width"] = boundingBox.width;
						jsonBoundingBox["height"] = boundingBox.height;
						jsonBoundingBox.push_back(jsonBoundingBox);
					}
				}
			}

			//----------
			void MarkerTagger::Capture::deserialize(const nlohmann::json & json) {
				{
					const auto & jsonMarkers = json["markers"];
					this->markers.clear();
					for (const auto & jsonMarker : jsonMarkers) {
						auto ID = jsonMarker["ID"].get<int>();
						auto x = jsonMarker["x"].get<float>();
						auto y = jsonMarker["y"].get<float>();
						this->markers.emplace(ID, cv::Point2f(x, y));
					}
				}
				
				{
					std::string filename;
					Utils::deserialize(json["imageFilename"], filename);
					if (!filename.empty()) {
						this->image = cv::imread(filename);
					}
				}

				{
					this->boundingBoxes.clear();
					const auto & jsonBoundingBoxes = json["boundingBoxes"];
					for (auto jsonBoundingBox : jsonBoundingBoxes) {
						auto x = jsonBoundingBox["x"].get<int>();
						auto y = jsonBoundingBox["y"].get<int>();
						auto width = jsonBoundingBox["width"].get<int>();
						auto height = jsonBoundingBox["height"].get<int>();
						this->boundingBoxes.emplace_back(x, y, width, height);
					}
				}
			}

			//----------
			void MarkerTagger::Capture::tagMarker(const glm::vec2 & imageCoordinate) {
				//remove any existing markers
				this->removeMarker(imageCoordinate);

				for (const auto boundingBox : this->boundingBoxes) {
					if (boundingBox.contains(ofxCv::toCv(imageCoordinate))) {					
						//get index
						auto IDString = ofSystemTextBoxDialog("Marker ID");
						if (!IDString.empty()) {
							auto ID = ofToInt(IDString);

							//find centroid
							auto moments = cv::moments(this->image(boundingBox));
							cv::Point2f centroid{ (float) (moments.m10 / moments.m00) + boundingBox.x
								, (float) (moments.m01 / moments.m00) + boundingBox.y };
							
							this->markers.emplace(ID, centroid);
						}
						break;
					}
				}
			}

			//----------
			void MarkerTagger::Capture::removeMarker(const glm::vec2 & imageCoordinate) {
				for (const auto boundingBox : this->boundingBoxes) {
					if (boundingBox.contains(ofxCv::toCv(imageCoordinate))) {
						for (auto markerIt = this->markers.begin(); markerIt != this->markers.end(); ) {
							if (boundingBox.contains(markerIt->second)) {
								markerIt = this->markers.erase(markerIt);
							}
							else {
								markerIt++;
							}
						}
						break;
					}
				}
			}

			//----------
			void MarkerTagger::Capture::drawOnImage() {
				ofPushStyle();
				{

					//draw filtered contours
					{
						ofSetColor(0, 255, 0);

						for (auto contour : this->filteredContours) {
							ofMesh line;
							line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
							for (auto vertex : contour) {
								line.addVertex(glm::vec3(vertex.x, vertex.y, 0.0f));
							}
							line.draw();
						}
					}

					//draw bounding boxes
					{
						ofNoFill();
						ofSetLineWidth(1.0f);

						for (auto boudingBox : this->boundingBoxes) {
							ofDrawRectangle(ofxCv::toOf(boudingBox));
						}
					}

					//draw tagged markers
					{
						for (const auto & marker : this->markers) {
							ofPushMatrix();
							{
								ofTranslate(ofxCv::toOf(marker.second));
								ofxCvGui::Utils::drawToolTip(ofToString(marker.first), glm::vec2(0, 0));
							}
							ofPopMatrix();
						}
					}
				}
				ofPopStyle();
			}

#pragma mark MarkerTagger
			//----------
			MarkerTagger::MarkerTagger() {
				RULR_NODE_INIT_LISTENER;
				this->parameters.previewType.addListener(this, &MarkerTagger::callbackPreviewMode);
			}

			//----------
			string MarkerTagger::getTypeName() const {
				return "MoCap::MarkerTagger";
			}

			//----------
			void MarkerTagger::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::Camera>();

				this->captures.onSelectionChanged += [this]() {
					this->updatePreview();
				};

				{
					auto panel = ofxCvGui::Panels::makeImage(this->preview);
					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						auto capture = this->getSelection();
						if (capture) {
							capture->drawOnImage();
						}
					};
					panel->onDraw += [this](ofxCvGui::DrawArguments & args) {
						stringstream status;
						for (auto iterator : this->count) {
							status << iterator.first << " seen in " << iterator.second << " views" << endl;
						}
						ofxCvGui::Utils::drawText(status.str(), 30, 90);
					};
					auto panelWeak = weak_ptr<ofxCvGui::Panels::Image>(panel);
					panel->onMouseReleased += [this, panelWeak](ofxCvGui::MouseArguments & args) {
						auto panel = panelWeak.lock();
						auto capture = this->getSelection();
						if (capture) {
							auto panelCoordinate = Utils::applyTransform(glm::inverse(panel->getPanelToImageTransform()), args.local);

							if (args.button == 0) {
								capture->tagMarker(panelCoordinate);
							}
							else {
								capture->removeMarker(panelCoordinate);
							}
						}
					};
					this->panel = panel;
				}
			}

			//----------
			void MarkerTagger::update() {
				//would be nice to do elsewhere, but this is safer for now
				this->updateCount();
			}

			//----------
			void MarkerTagger::serialize(nlohmann::json & json) {
				Utils::serialize(json, this->parameters);
				this->captures.serialize(json);
			}

			//----------
			void MarkerTagger::deserialize(const nlohmann::json & json) {
				Utils::deserialize(json, this->parameters);
				this->captures.deserialize(json);
			}

			//----------
			void MarkerTagger::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Add capture", [this]() {
					try {
						this->addCapture();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, ' ');

				this->captures.populateWidgets(inspector);
				inspector->addParameterGroup(this->parameters);
			}

			//----------
			ofxCvGui::PanelPtr MarkerTagger::getPanel() {
				return this->panel;
			}

			//----------
			void MarkerTagger::addCapture() {
				this->throwIfMissingAConnection<Item::Camera>();
				auto cameraNode = this->getInput<Item::Camera>();

				auto frame = cameraNode->getFrame();

				if (!frame) {
					throw(ofxRulr::Exception("No camera frame available"));
				}

				auto capture = make_shared<Capture>();
				ofxCv::copy(frame->getPixels(), capture->image);

				//local difference
				{
					//iterative blur
					{
						int blurSize = this->parameters.localDifference.blurSize;

						cv::blur(capture->image, capture->blurred, cv::Size(blurSize / 2, blurSize / 2));
						blurSize /= 2;
						while (blurSize > 1) {
							if (blurSize <= 32) {
								cv::blur(capture->blurred, capture->blurred, cv::Size(blurSize, blurSize));
								break;
							}
							cv::blur(capture->blurred, capture->blurred, cv::Size(blurSize / 2, blurSize / 2));
							blurSize /= 2;
						}
					}

					capture->difference = capture->image - capture->blurred;
					capture->difference *= this->parameters.localDifference.differenceAmplify;

					cv::threshold(capture->difference
						, capture->binary
						, this->parameters.localDifference.threshold
						, 255
						, cv::THRESH_BINARY);
				}

				//contour and bounding boxes
				{
					cv::findContours(capture->binary
						, capture->contours
						, cv::RETR_EXTERNAL
						, cv::CHAIN_APPROX_NONE);

					auto minimumArea = this->parameters.contourFilter.minimumArea.get();
					minimumArea *= minimumArea;
					auto maximumArea = this->parameters.contourFilter.maximumArea.get();
					maximumArea *= maximumArea;
					for (const auto & contour : capture->contours) {
						const auto area = cv::contourArea(contour);
						if (area >= minimumArea && area <= maximumArea) {
							capture->filteredContours.push_back(contour);
							capture->boundingBoxes.emplace_back(cv::boundingRect(contour));
						}
					}
				}

				capture->timestamp = frame->getTimestamp();

				this->captures.add(capture);
				this->captures.select(capture);
				
				this->updatePreview();
			}

			//----------
			void MarkerTagger::updatePreview() {
				auto capture = this->getSelection();

				if (capture) {
					cv::Mat * previewMatrix = nullptr;
					switch (this->parameters.previewType.get()) {
					case PreviewType::Raw:
						previewMatrix = &capture->image;
						break;
					case PreviewType::Blurred:
						previewMatrix = &capture->blurred;
						break;
					case PreviewType::Diff:
						previewMatrix = &capture->difference;
						break;
					case PreviewType::Binary:
						previewMatrix = &capture->binary;
						break;
					default:
						break;
					}

					if (previewMatrix) {

						//try again with the raw image (e.g. we don't serialise the other images)
						if (previewMatrix->empty()) {
							previewMatrix = &capture->image;
						}

						ofxCv::copy(* previewMatrix, this->preview.getPixels());
						this->preview.update();
					}
					else {
						this->preview.clear();
					}
				}
				else {
					this->preview.clear();
				}
			}

			//----------
			void MarkerTagger::updateCount() {
				auto captures = this->captures.getAllCaptures();
				map<int, size_t> count;
				for (auto capture : captures) {
					for (auto marker : capture->markers) {
						auto findCount = count.find(marker.first);
						if(findCount == count.end()) {
							count[marker.first] = 1;
						}
						else {
							findCount->second++;
						}
					}
				}
				this->count = move(count);
			}

			//----------
			void MarkerTagger::callbackPreviewMode(PreviewType &) {
				this->updatePreview();
			}

			//----------
			shared_ptr<MarkerTagger::Capture> MarkerTagger::getSelection() {
				auto selectionSet = this->captures.getSelection();
				if (selectionSet.empty()) {
					return shared_ptr<Capture>();
				}
				else {
					return *selectionSet.begin();
				}
			}
		}
	}
}
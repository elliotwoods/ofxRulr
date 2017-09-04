#include "pch_Plugin_MoCap.h"
#include "PreviewCentroids.h"


namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			//----------
			PreviewCentroids::PreviewCentroids() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			PreviewCentroids::~PreviewCentroids() {

			}

			//----------
			std::string PreviewCentroids::getTypeName() const {
				return "MoCap::PreviewCentroids";
			}

			//----------
			void PreviewCentroids::init() {
				RULR_NODE_UPDATE_LISTENER;

				auto panel = ofxCvGui::Panels::makeImage(this->previewImage);
				panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
					{
						this->previewFrameMutex.lock();
						auto previewFrame = this->previewFrame;
						this->previewFrameMutex.unlock();

						if(previewFrame) {
							ofPushStyle();
							{
								if (this->parameters.drawCentroids) {
									ofSetColor(255, 0, 0);
									for (const auto & centroid : previewFrame->centroids) {
										ofPushMatrix();
										{
											ofTranslate(centroid.x, centroid.y);
											ofDrawLine(-10, 0, 10, 0);
											ofDrawLine(0, -10, 0, 10);
										}
										ofPopMatrix();
									}
								}

								if (this->parameters.drawBounds) {
									ofNoFill();
									ofSetLineWidth(1.0f);
									ofSetColor(100, 255, 0);
									for (const auto & boundingRect : previewFrame->boundingRects) {
										ofDrawRectangle(ofxCv::toOf(boundingRect));
									}
								}

								if (this->parameters.drawText) {
									ofSetColor(255);
									for (int i = 0; i < previewFrame->boundingRects.size(); i++) {
										auto & rect = previewFrame->boundingRects[i];

										//measure brightness
										float brightness;
										{
											auto image = ofxCv::toCv(previewFrame->imageFrame->getPixels());

											auto measureRect = rect;
											measureRect.width /= 2.0f;
											measureRect.height /= 2.0f;
											measureRect.x += measureRect.width / 2;
											measureRect.y += measureRect.height / 2;
											auto measurement = cv::mean(image(measureRect));
											brightness = measurement[0];
										}

										stringstream markerInfo;
										markerInfo << "Area : " << rect.area() << endl;
										markerInfo << "Circularity : " << previewFrame->circularity[i] << endl;
										markerInfo << "Brightness : " << brightness << endl;
										ofDrawBitmapString(markerInfo.str(), rect.x + rect.width, rect.y + rect.height + 10, false);
									}
								}

							}
							ofPopStyle();
						}
					}
				};
				panel->onDraw += [this](ofxCvGui::DrawArguments & args) {
					{
						this->previewFrameMutex.lock();
						auto previewFrame = this->previewFrame;
						this->previewFrameMutex.unlock();
						if (previewFrame) {
							stringstream status;
							status << "Marker count : " << previewFrame->centroids.size();
							ofxCvGui::Utils::drawText(status.str(), 20, 70, false);
						}
					}
				};
				this->panel = panel;

				this->manageParameters(this->parameters);
			}

			//----------
			void PreviewCentroids::update() {
				if (this->previewImageDirty.load()) {
					this->previewFrameMutex.lock();
					auto previewFrame = this->previewFrame;
					this->previewFrameMutex.unlock();

					if (previewFrame) {
						this->previewImage.getPixels() = previewFrame->imageFrame->getPixels();
						cv::Mat masked = ofxCv::toCv(previewFrame->imageFrame->getPixels()) & previewFrame->binary;
						ofxCv::copy(masked, this->previewImage.getPixels());
						this->previewImage.update();
					}
					else {
						this->previewImage.clear();
					}

					this->previewImageDirty.store(false);
				}
			}

			//----------
			ofxCvGui::PanelPtr PreviewCentroids::getPanel() {
				return this->panel;
			}

			//----------
			void PreviewCentroids::processFrame(shared_ptr<FindMarkerCentroidsFrame> incomingFrame) {
				auto lock = unique_lock<mutex>(this->previewFrameMutex);
				this->previewFrame = incomingFrame;
				this->previewImageDirty.store(true);
			}

		}
	}
}
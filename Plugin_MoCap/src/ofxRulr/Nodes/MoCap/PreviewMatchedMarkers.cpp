#include "pch_Plugin_MoCap.h"
#include "PreviewMatchedMarkers.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
			//----------
			PreviewMatchedMarkers::PreviewMatchedMarkers() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string PreviewMatchedMarkers::getTypeName() const {
				return "MoCap::PreviewMatchedMarkers";
			}

			//----------
			void PreviewMatchedMarkers::init() {
				RULR_NODE_UPDATE_LISTENER;

				auto panel = ofxCvGui::Panels::makeImage(this->preview);
				panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
					this->previewFrameMutex.lock();
					auto previewFrame = this->previewFrame;
					this->previewFrameMutex.unlock();
					
					if (previewFrame) {
						//draw a background
						ofPushStyle();
						{
							ofEnableAlphaBlending();
							ofSetColor(0, 200);
							ofDrawRectangle(args.drawBounds);
						}
						ofPopStyle();

						//preview projected points
						ofPushStyle();
						{
							ofSetColor(255, 0, 0);
							ofSetLineWidth(1.0f);

							//check it's not empty first (e.g. when doing exhaustive search)
							if (!previewFrame->projectedMarkerImagePoints.empty()) {
								auto size = previewFrame->bodyDescription->markerCount;
								for (auto i = 0; i < size; i++) {
									const auto & projectedPoint = previewFrame->projectedMarkerImagePoints[i];
									const auto & ID = previewFrame->bodyDescription->markers.IDs[i];
									ofPushMatrix();
									{
										ofTranslate(ofxCv::toOf(projectedPoint));
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);
										ofDrawBitmapString(ofToString(ID), 0, 0);
									}
									ofPopMatrix();
								}
							}
						}
						ofPopStyle();

						//preview marker centroids
						ofPushStyle();
						{
							ofSetColor(0, 255, 0);
							ofSetLineWidth(1.0f);
							for (auto centroid : previewFrame->incomingFrame->centroids) {
								ofPushMatrix();
								{
									ofTranslate(ofxCv::toOf(centroid));
									ofDrawLine(-10, 0, 10, 0);
									ofDrawLine(0, -10, 0, 10);
								}
								ofPopMatrix();
							}
						}
						ofPopStyle();

						//preview matches
						ofPushStyle();
						{
							ofSetColor(0, 100, 255);
							ofSetLineWidth(1.0f);
							for (int i = 0; i < previewFrame->matchCount; i++) {
								ofDrawLine(ofxCv::toOf(previewFrame->matchedProjectedPoint[i])
									, ofxCv::toOf(previewFrame->matchedCentroids[i]));
								ofDrawBitmapString("Marker #" + ofToString(previewFrame->matchedMarkerID[i])
									, ofxCv::toOf(previewFrame->matchedProjectedPoint[i]));
							}
						}
						ofPopStyle();
					}
				};
				this->panel = panel;
			}

			//----------
			void PreviewMatchedMarkers::update() {
				if (this->previewDirty) {
					auto lock = unique_lock<mutex>(this->previewFrameMutex);
					if (this->previewFrame) {
						ofxCv::copy(this->previewFrame->incomingFrame->binaryImage, this->preview.getPixels());
						this->preview.update();
					}
					else {
						this->preview.clear();
					}
					this->previewDirty.store(false);
				}
			}

			//----------
			ofxCvGui::PanelPtr PreviewMatchedMarkers::getPanel() {
				return this->panel;
			}

			//----------
			void PreviewMatchedMarkers::processFrame(shared_ptr<MatchMarkersFrame> incomingFrame) {
				auto lock = unique_lock<mutex>(this->previewFrameMutex);
				this->previewFrame = incomingFrame;
				this->previewDirty = true;
			}
		}
	}
}
#include "pch_Plugin_MoCap.h"
#include "PreviewRecordMarkerImageFrame.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {

			//----------
			PreviewRecordMarkerImagesFrame::PreviewRecordMarkerImagesFrame() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string PreviewRecordMarkerImagesFrame::getTypeName() const {
				return "MoCap::PreviewRecordMarkerImagesFrame";
			}

			//----------
			void PreviewRecordMarkerImagesFrame::init() {
				RULR_NODE_UPDATE_LISTENER;

				auto panelGroup = ofxCvGui::Panels::Groups::makeGrid();
				{
					auto imagePanel = ofxCvGui::Panels::makeImage(this->image, "Image");
					imagePanel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
						this->drawOnImage(args);
					};
					panelGroup->add(imagePanel);

					panelGroup->add(ofxCvGui::Panels::makeImage(this->blurred, "Blurred"));
					panelGroup->add(ofxCvGui::Panels::makeImage(this->difference, "Difference"));
					panelGroup->add(ofxCvGui::Panels::makeImage(this->binary, "Binary"));
				}
				this->panel = panelGroup;
			}

			//----------
			void PreviewRecordMarkerImagesFrame::update() {
				shared_ptr<RecordMarkerImagesFrame> previewFrame;
				{
					auto lock = unique_lock<mutex>(this->previewFrameMutex);
					previewFrame = this->previewFrame;
					this->previewFrame.reset();
				}

				if (previewFrame) {
					ofxCv::copy(previewFrame->image, this->image.getPixels());
					ofxCv::copy(previewFrame->blurred, this->blurred.getPixels());
					ofxCv::copy(previewFrame->difference, this->difference.getPixels());
					ofxCv::copy(previewFrame->binary, this->binary.getPixels());

					this->image.update();
					this->blurred.update();
					this->difference.update();
					this->binary.update();

					vector<ofMesh> contours;
					for (const auto & contour : previewFrame->contours) {
						ofMesh contourMesh;
						contourMesh.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
						for (auto point : contour) {
							contourMesh.addVertex(ofVec3f(point.x, point.y));
						}
						contours.push_back(move(contourMesh));
					}
					this->contours = move(contours);

					vector<ofPath> filteredContours;
					for (const auto & contour : previewFrame->filteredContours) {
						ofPath line;
						for (auto point : contour) {
							line.lineTo(point.x, point.y);
						}
						line.close();
						filteredContours.push_back(move(line));
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr PreviewRecordMarkerImagesFrame::getPanel() {
				return this->panel;
			}

			//----------
			void PreviewRecordMarkerImagesFrame::processFrame(shared_ptr<RecordMarkerImagesFrame> incomingFrame) {
				{
					auto lock = unique_lock<mutex>(this->previewFrameMutex);
					this->previewFrame = incomingFrame;
				}
				this->onNewFrame(shared_ptr<void *>());
			}

			//----------
			void PreviewRecordMarkerImagesFrame::drawOnImage(ofxCvGui::DrawImageArguments & args) {
				ofPushStyle();
				{
					ofSetColor(255, 0, 0);
					for (auto contour : this->contours) {
						contour.draw();
					}

					ofFill();
					ofEnableAlphaBlending();
					ofSetColor(0, 255, 0, 100);
					for (auto contour : this->filteredContours) {
						contour.draw();
					}
				}
				ofPopStyle();
			}

		}
	}
}
#include "pch_Plugin_Reworld.h"
#include "CameraTest.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//---------
			CameraTest::CameraTest()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string
				CameraTest::getTypeName() const
			{
				return "Reworld::CameraTest";
			}
			
			//---------
			void
				CameraTest::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				{
					auto panel = ofxCvGui::Panels::makeImage(this->preview);

					panel->onDrawImage += [this](ofxCvGui::DrawImageArguments& args) {
						ofPushMatrix();
						{
							ofMultMatrix(this->homography);
							ofxAssets::image("ofxRulr::Reworld::Shroud").draw(0, 0);
						}
						ofPopMatrix();

						ofPushStyle();
						{
							for (int i = 0; i < 4; i++) {
								auto selected = this->isBeingInspected() && i == this->selectedCornerIndex;
								const auto& targetCorner = this->targetCorners[i];
								if (selected) {
									ofSetColor(ofxCvGui::Utils::getBeatingSelectionColor());
								}
								else {
									ofSetColor(255, 255, 255);
								}
								ofPushMatrix();
								{
									ofTranslate(targetCorner);
									ofDrawLine(-10, 0, 10, 0);
									ofDrawLine(0, -10, 0, 10);
								}
								ofPopMatrix();
							}
						}
						ofPopStyle();
					};

					auto panelWeak = weak_ptr<ofxCvGui::Panels::Image>(panel);

					panel->onKeyboard += [this](ofxCvGui::KeyboardArguments& args) {

						if (args.action == ofxCvGui::KeyboardArguments::Action::Pressed
							&& this->isBeingInspected()) {
							auto& targetCorner = this->targetCorners[this->selectedCornerIndex];
							auto movement = 1;
							switch (args.key) {
							case OF_KEY_LEFT:
								targetCorner.x -= movement;
								break;
							case OF_KEY_RIGHT:
								targetCorner.x += movement;
								break;
							case OF_KEY_UP:
								targetCorner.y -= movement;
								break;
							case OF_KEY_DOWN:
								targetCorner.y += movement;
								break;
							case OF_KEY_TAB:
								if (ofGetKeyPressed(OF_KEY_SHIFT)) {
									this->selectedCornerIndex--;
									if (this->selectedCornerIndex < 0) {
										this->selectedCornerIndex = 3;
									}
								}
								else {
									this->selectedCornerIndex++;
									if (this->selectedCornerIndex > 3) {
										this->selectedCornerIndex = 0;
									}
								}
							deafult:
								break;
							}
							this->transformDirty = true;
						}
					};

					panel->onMouse += [this, panelWeak](ofxCvGui::MouseArguments& args) {
						if (!this->isBeingInspected()) {
							return;
						}

						auto panel = panelWeak.lock();
						args.takeMousePress(panel);

						if (args.isDragging(panel)) {
							auto& targetCorner = this->targetCorners[selectedCornerIndex];

							auto beforeMovementPanel = args.local - args.movement;
							auto afterMovementPanel = args.local;

							auto imageToPanel = glm::inverse(panel->getPanelToImageTransform());

							auto beforeMovementImage = ofxCeres::VectorMath::applyTransform(imageToPanel, glm::vec3(beforeMovementPanel, 1.0f));
							auto afterMovementImage = ofxCeres::VectorMath::applyTransform(imageToPanel, glm::vec3(afterMovementPanel, 1.0f));

							auto movementImage = afterMovementImage - beforeMovementImage;

							if (ofGetKeyPressed(OF_KEY_SHIFT)) {
								movementImage *= 0.2f;
							}

							targetCorner += { movementImage.x, movementImage.y };
							this->transformDirty = true;
						}
					};

					this->panel = panel;
				}

				this->sourceCorners = {
					{ 125, 25 }
					, { 386, 25 }
					, { 386, 486 }
					, { 125, 486 }
				};

				// initialise target as same as source for test
				this->targetCorners = this->sourceCorners;

				this->addInput<Item::Camera>();
			}

			//---------
			void
				CameraTest::update()
			{
				auto camera = this->getInput<Item::Camera>();
				if (camera) {
					auto grabber = camera->getGrabber();
					if (grabber) {
						if (grabber->isFrameNew()) {
							// copy the preview instead of undistorting again here
							this->preview = camera->getUndistortedPreview();
						}
					}
				}

				if (this->transformDirty) {
					auto homography = ofxCv::findHomography(ofxCv::toCv(this->sourceCorners), ofxCv::toCv(this->targetCorners));
					cout << homography << endl;
					this->homography[0][0] = homography.at<double>(0, 0);
					this->homography[1][0] = homography.at<double>(1, 0);
					this->homography[2][0] = 0.0f;
					this->homography[3][0] = homography.at<double>(2, 0);

					this->homography[0][1] = homography.at<double>(0, 1);
					this->homography[1][1] = homography.at<double>(1, 1);
					this->homography[2][1] = 0.0f;
					this->homography[3][1] = homography.at<double>(2, 1);

					this->homography[2][0] = 0.0f;
					this->homography[2][1] = 0.0f;
					this->homography[2][2] = 1.0f;
					this->homography[2][3] = 0.0f;

					this->homography[0][3] = homography.at<double>(0, 2);
					this->homography[1][3] = homography.at<double>(1, 2);
					this->homography[2][3] = 0.0f;
					this->homography[3][3] = 1.0f;

					this->homography = glm::transpose(this->homography);

					this->transformDirty = false;
					cout << this->homography << endl;
				}
			}

			//---------
			void
				CameraTest::populateInspector(ofxCvGui::InspectArguments& args)
			{

			}

			//---------
			ofxCvGui::PanelPtr
				CameraTest::getPanel()
			{
				return this->panel;
			}
		}
	}
}
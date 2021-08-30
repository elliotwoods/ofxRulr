#include "pch_Plugin_ArUco.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			//----------
			Transform::Transform() 
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				Transform::getTypeName() const
			{
				return "MarkerMap::Transform";
			}

			//----------
			void
				Transform::init()
			{
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->manageParameters(this->parameters);

				this->addInput<Nodes::Base>("Draw");
				this->addInput<MarkerMap::NavigateCamera>();

				{
					auto panel = ofxCvGui::Panels::makeImage(this->preview, "Preview");
					this->panel = panel;
				}

				if (!this->parameters.filename.get().empty()) {
					this->loadPhoto();
				}
			}

			//----------
			void
				Transform::update()
			{
				auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();

				bool shouldPerform;
				auto whenShouldPerform = this->parameters.autoUpdate.get();
				if (whenShouldPerform == WhenDrawOnWorldStage::Always
					|| (this->isBeingInspected() && whenShouldPerform == WhenDrawOnWorldStage::Selected)) {
					shouldPerform = true;
				}
				if (navigateCamera) {
					auto camera = navigateCamera->getInput<Item::Camera>();
					if (!camera) {
						shouldPerform = false;
					}
				}
				shouldPerform &= this->photo.isAllocated();

				if (shouldPerform) {
					this->updatePreview();
				}
			}

			//----------
			void
				Transform::drawWorldStage()
			{
				auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();
				if (!navigateCamera) {
					return;
				}

				auto markersNode = navigateCamera->getInput<MarkerMap::Markers>();
				if (!markersNode) {
					return;
				}

				ofPushMatrix();
				{
					ofMultMatrix(this->getTransform());
					markersNode->drawWorldStage();
				}
				ofPopMatrix();
			}

			//----------
			void
				Transform::populateInspector(ofxCvGui::InspectArguments& inspectArgs)
			{
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Load image...", [this]() {
					try {
						auto result = ofSystemLoadDialog("Select photo");

						if (result.bSuccess) {
							this->parameters.filename.set(result.filePath);
							this->loadPhoto();
						}
					}
					RULR_CATCH_ALL_TO_ALERT
					});

				inspector->addButton("Navigate", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Navigate camera");
						this->navigate();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT
					});

				inspector->addButton("Update preview", [this]() {
					try {
						this->updatePreview();
					}
					RULR_CATCH_ALL_TO_ALERT
					});

				inspector->addButton("Bake transform", [this]() {
					try {
						this->bakeTransform();
					}
					RULR_CATCH_ALL_TO_ALERT
					});
			}

			//----------
			void
				Transform::serialize(nlohmann::json&)
			{
			}

			//----------
			void
				Transform::deserialize(const nlohmann::json&)
			{
			}

			//----------
			ofxCvGui::PanelPtr 
				Transform::getPanel()
			{
				return this->panel;
			}

			//----------
			void
				Transform::loadPhoto()
			{
				auto image = cv::imread(this->parameters.filename);
				if (image.type() == CV_16U) {
					image.convertTo(image, CV_8U, 1 / 256.0f);
				}
				ofxCv::copy(image, this->photo.getPixels());
				this->photo.update();
			}

			//----------
			void
				Transform::navigate()
			{
				this->throwIfMissingAConnection<MarkerMap::NavigateCamera>();
				auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();

				if (!this->photo.isAllocated()) {
					throw(ofxRulr::Exception("No photo. Please load one first."));
				}

				navigateCamera->throwIfMissingAnyConnection();
				auto camera = navigateCamera->getInput<Item::Camera>();
				
				if (this->photo.getWidth() != camera->getWidth()
					|| this->photo.getHeight() != camera->getHeight()) {
					throw(ofxRulr::Exception("Dimensions of photo do not match camera"));
				}

				cv::Mat image = ofxCv::toCv(this->photo.getPixels());

				navigateCamera->track(image, false);
			}

			//----------
			void
				Transform::updatePreview()
			{
				this->throwIfMissingAConnection<MarkerMap::NavigateCamera>();
				auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();

				auto camera = navigateCamera->getInput<Item::Camera>();
				if (!camera) {
					throw(ofxRulr::Exception("There must be a camera attached to the NavigateCamera node"));
				}

				// Check resolution
				auto width = camera->getWidth();
				auto height = camera->getHeight();

				// Allocate fbo
				if (this->fbo.getWidth() != width
					|| this->fbo.getHeight() != height) {
					ofFbo::Settings fboSettings;
					fboSettings.width = width;
					fboSettings.height = height;
					fboSettings.useDepth = true;
					fboSettings.depthStencilInternalFormat = GL_DEPTH_COMPONENT24;
					fboSettings.minFilter = GL_NEAREST;
					fboSettings.maxFilter = GL_NEAREST;
					this->fbo.allocate(fboSettings);
				}

				// Draw scene into fbo
				{
					this->fbo.begin();
					{
						ofClear(0, 0);
						auto cameraView = camera->getViewInWorldSpace();
						ofEnableDepthTest();
						cameraView.beginAsCamera();
						{
							ofPushMatrix();
							{
								ofMultMatrix(glm::inverse(this->getTransform()));
								auto nodeToDraw = this->getInput<Nodes::Base>("Draw");
								if (nodeToDraw) {
									nodeToDraw->drawWorldStage();
								}
							}
							ofPopMatrix();
						}
						cameraView.endAsCamera();
						ofDisableDepthTest();
					}
					this->fbo.end();
				}

				// Read back into pixels and composite
				{
					this->fbo.readToPixels(this->fboReadBack);
					cv::Mat drawing = ofxCv::toCv(this->fboReadBack);
					cv::flip(drawing
						, drawing
						, 0);

					// Extract RGB
					cv::Mat drawingRGB;
					cv::cvtColor(drawing, drawingRGB, cv::COLOR_RGBA2RGB);

					// extract alpha channel
					cv::Mat alphaMask;
					cv::extractChannel(drawing, alphaMask, 3);
					
					// create a contact shadow
					auto element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
					//element = element / 4;
					for (int i = 0; i < 4; i++) {
						cv::dilate(alphaMask, alphaMask, element);
					}

					// Copy photo into preview
					this->preview = this->photo;

					auto previewImage = ofxCv::toCv(this->preview.getPixels());

					// expand the alpha mask into 3 channels
					cv::Mat shadow;
					cv::cvtColor(alphaMask, shadow, cv::COLOR_GRAY2RGB);

					cv::addWeighted(previewImage
						, 1.0
						, shadow
						, -1.0
						, 0.0
						, previewImage);

					cv::copyTo(drawingRGB, previewImage, alphaMask);

					this->preview.update();
				}
			}

			//----------
			void
				Transform::bakeTransform()
			{
				this->throwIfMissingAConnection<MarkerMap::NavigateCamera>();
				auto navigateCamera = this->getInput<MarkerMap::NavigateCamera>();

				navigateCamera->throwIfMissingAConnection<MarkerMap::Markers>();
				auto markersNode = navigateCamera->getInput<MarkerMap::Markers>();
				
				auto markers = markersNode->getMarkers();

				auto transform = this->getTransform();

				for (auto marker : markers) {
					auto markerTransform = marker->rigidBody->getTransform();
					markerTransform = transform * markerTransform;
					marker->rigidBody->setTransform(markerTransform);
				}

				this->parameters.translation.x.set(0);
				this->parameters.translation.y.set(0);
				this->parameters.translation.z.set(0);
				this->parameters.rotation.x.set(0);
				this->parameters.rotation.y.set(0);
				this->parameters.rotation.z.set(0);
				this->parameters.scale.x.set(1);
				this->parameters.scale.y.set(1);
				this->parameters.scale.z.set(1);
			}

			//----------
			glm::mat4
				Transform::getTransform() const
			{
				glm::vec3 translation(this->parameters.translation.x.get()
					, this->parameters.translation.y.get()
					, this->parameters.translation.z.get());
				glm::vec3 rotation(this->parameters.rotation.x.get()
					, this->parameters.rotation.y.get()
					, this->parameters.rotation.z.get());
				glm::vec3 scale(this->parameters.scale.x.get()
					, this->parameters.scale.y.get()
					, this->parameters.scale.z.get());

				return glm::translate(translation)
					* glm::mat4(glm::quat(rotation * DEG_TO_RAD))
					* glm::scale(scale);
			}
		}
	}
}
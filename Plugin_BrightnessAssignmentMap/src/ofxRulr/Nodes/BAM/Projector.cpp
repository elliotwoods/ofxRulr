#include "pch_Plugin_BrightnessAssignmentMap.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			//----------
			Pass::Pass() {
				this->fbo = make_shared<ofFbo>();
			}

			//----------
			Pass::Pass(ofFbo::Settings settings)
			: Pass() {
				this->settings = settings;
			}

			//----------
			string Pass::toString(Level level) {
				switch (level) {
				case Color:
					return "Color";
				case Depth:
					return "Depth";
				case DepthPreview:
					return "DepthPreview";
				case AvailabilityProjection:
					return "AvailabilityProjection";
				case AccumulateAvailability:
					return "AccumulateAvailability";
				case BrightnessAssignmentMap:
					return "BrightnessAssignmentMap";
				case End:
					return "End";
				default:
				case None:
					return "None";
				}
			}

			//----------
			Projector::Projector() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Projector::getTypeName() const {
				return "BAM::Projector";
			}

			//----------
			void BAM::Projector::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::Projector>();

				{
					ofFbo::Settings color2D;
					{
						color2D.useDepth = false;
						color2D.internalformat = GL_RGBA;
					}

					ofFbo::Settings color3D;
					{
						color3D.useDepth = true;
						color3D.depthStencilAsTexture = true;
						color3D.internalformat = GL_RGBA;
						color3D.depthStencilInternalFormat = GL_DEPTH_COMPONENT32;
					}

					ofFbo::Settings oneChannel2D;
					{
						oneChannel2D.useDepth = false;
						oneChannel2D.internalformat = GL_R32F;
					}

					ofFbo::Settings oneChannel3D;
					{
						oneChannel3D.useDepth = true;
						oneChannel3D.depthStencilAsTexture = true;
						oneChannel3D.internalformat = GL_R32F;
						oneChannel3D.depthStencilInternalFormat = GL_DEPTH_COMPONENT32;
					}
					
					this->passes = Passes{
						{ Pass::Level::Color, color3D },
						//{ Pass::Level::Depth, depth3D },
						{ Pass::Level::DepthPreview, color2D },
						{ Pass::Level::AvailabilityProjection, oneChannel3D },
						{ Pass::Level::AccumulateAvailability, oneChannel2D },
						{ Pass::Level::BrightnessAssignmentMap, oneChannel2D }
					};
				}
				
				{
					auto worldInput = this->addInput<BAM::World>();
					worldInput->onNewConnection += [this](shared_ptr<World> world) {
						world->registerProjector(static_pointer_cast<Projector>(this->shared_from_this()));
					};
					worldInput->onDeleteConnection += [this](shared_ptr<World> world) {
						if (world) {
							world->unregisterProjector(static_pointer_cast<Projector>(this->shared_from_this()));
						}
					};
				}

				{
					this->panelGroup = ofxCvGui::Panels::Groups::makeGrid();
					{
						for (const auto & pass : this->passes) {
							auto panel = ofxCvGui::Panels::makeBaseDraws(* pass.second.fbo, Pass::toString(pass.first));
							this->panelGroup->add(panel);
							auto passLevel = pass.first;
							panel->addToolBarElement("ofxCvGui::save", [this, passLevel] {
								this->exportPass(passLevel);
							});
						}
					}
				}
				this->manageParameters(this->parameters);
			}

			//----------
			void Projector::update() {
				if (this->parameters.autoRender) {
					this->render(Pass::Level::End);
				}
			}

			//----------
			ofxCvGui::PanelPtr Projector::getPanel() {
				return this->panelGroup;
			}

			//----------
			void Projector::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Export BAM...", [this]() {
					try {
						this->exportPass(Pass::Level::BrightnessAssignmentMap);
					}
					RULR_CATCH_ALL_TO_ALERT;
				})->setHeight(100.0f);
			}

			//----------
			const Passes & Projector::getPasses(Pass::Level requiredPass) {
				this->render(requiredPass);
				return this->passes;
			}

			//----------
			Pass & Projector::getPass(Pass::Level passLevel, bool ensureRendered /*= true*/) {
				if (ensureRendered) {
					this->render(passLevel);
				}
				return this->passes[passLevel];
			}

			//----------
			void Projector::render(Pass::Level requiredPass) {
				auto projector = this->getInput<Item::Projector>();
				auto world = this->getInput<World>();
				auto frameIndex = ofGetFrameNum();

				if (projector && world) {
					auto scene = world->getInput<Nodes::Base>("Scene");
					if (scene) {
						auto view = projector->getViewInWorldSpace();

						auto width = projector->getWidth();
						auto height = projector->getHeight();

						ofPlanePrimitive plane;
						plane.set(width, height, 2, 2);
						plane.setPosition(ofVec3f(width / 2.0f, height / 2.0f, 0.0f));

						{
							//go through requires levels
							for (uint8_t i = 0; i <= requiredPass; i++) {
								auto findPass = this->passes.find((Pass::Level) i);
								if (findPass == this->passes.end()) {
									continue;
								}
								auto & pass = findPass->second;

								//check if already rendered this frame
								auto currentFrameIndex = ofGetFrameNum();
								if (pass.renderedFrameIndex == currentFrameIndex) {
									continue;
								}

								//allocate fbo
								if (pass.fbo->getWidth() != width
									|| pass.fbo->getHeight() != height) {
									ofFbo::Settings fboSettings = pass.settings;
									fboSettings.width = projector->getWidth();
									fboSettings.height = projector->getHeight();
									pass.fbo->allocate(fboSettings);
								}

								plane.mapTexCoordsFromTexture(pass.fbo->getTexture());

								//draw fbo
								pass.fbo->begin();
								{
									ofClear(0, 0);

									switch (i) {
									case Pass::Level::None:
										break;
									case Pass::Level::Color:
									{
										view.beginAsCamera();
										{
											ofxRulr::Utils::Graphics::glEnable(GL_DEPTH_TEST);
											{
												ofxRulr::DrawWorldAdvancedArgs args;
												args.enableGui = false;
												args.enableStyle = true;
												args.enableMaterials = true;
												args.enableColors = true;
												args.enableTextures = true;
												args.enableNormals = true;
												scene->drawWorldAdvanced(args);
											}
											ofxRulr::Utils::Graphics::glDisable(GL_DEPTH_TEST);
										}
										view.endAsCamera();
										break;
									}
									case Pass::Level::Depth:
									{
										view.beginAsCamera();
										ofxRulr::Utils::Graphics::glEnable(GL_DEPTH_TEST);
										{
											auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::Depth");
											shader.begin();
											{
												ofxRulr::Utils::Graphics::glEnable(GL_DEPTH_TEST);
												{
													ofxRulr::DrawWorldAdvancedArgs args;
													args.enableGui = false;
													args.enableStyle = false;
													args.enableMaterials = false;
													args.enableColors = false;
													args.enableTextures = false;
													args.enableNormals = false;
													scene->drawWorldAdvanced(args);
												}
												ofxRulr::Utils::Graphics::glDisable(GL_DEPTH_TEST);
											}
											shader.end();
										}
										ofxRulr::Utils::Graphics::glDisable(GL_DEPTH_TEST);
										view.endAsCamera();
										break;
									}
									case Pass::Level::DepthPreview:
									{
										auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::DepthPreview");
										auto & depthTexture = this->passes[Pass::Level::Color].fbo->getDepthTexture();
										shader.begin();
										{
											shader.setUniformTexture("depthTexture", depthTexture, 0);
											shader.setUniformTexture("depthTextureShadow", depthTexture, 1);
											plane.draw();
										}
										shader.end();
										break;
									}
									case Pass::Level::AvailabilityProjection:
										this->renderAvailability(static_pointer_cast<Projector>(shared_from_this()), world, view, scene);
										break;
									case Pass::Level::AccumulateAvailability:
									{
										//Unbind, since we shouldn't be binded to the pass since we need to render to some other buffers first
										pass.fbo->unbind();
										{
											auto allProjectors = world->getProjectors();
											auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::AvailabilityProjection");
											auto thisProjector = static_pointer_cast<Projector>(shared_from_this());
											
											//Setup a temporary fbo to draw each into
											ofFbo availabilityFromOtherProjector;
											{
												ofFbo::Settings fboSettings;
												fboSettings.width = width;
												fboSettings.height = height;
												fboSettings.useDepth = true;
												fboSettings.depthStencilAsTexture = true;
												fboSettings.internalformat = GL_R32F;
												fboSettings.depthStencilInternalFormat = GL_DEPTH_COMPONENT32;
												availabilityFromOtherProjector.allocate(fboSettings);
											}

											//Add the contribution from the other projectors
											for (auto otherProjector : allProjectors) {
												//Skip this projector
												if (otherProjector == thisProjector) {
													continue;
												}

												//Draw the 3D scene into fbo with Availability Projection shader
												availabilityFromOtherProjector.begin();
												{
													ofClear(0, 0);
													this->renderAvailability(otherProjector, world, view, scene);
												}
												availabilityFromOtherProjector.end();

												//Add the result of the render to the accumulation buffer
												pass.fbo->begin();
												{
													ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
													{
														availabilityFromOtherProjector.draw(0, 0);
													}
													ofDisableBlendMode();
												}
												pass.fbo->end();
											}

											//Finally let's add the contribution from this projector which we calculated in previous pass
											{
												pass.fbo->begin();
												{
													ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
													{
														this->passes[Pass::Level::AvailabilityProjection].fbo->draw(0, 0);
													}
													ofDisableBlendMode();
												}
												pass.fbo->end();
											}
										}

										//Undo unbind
										pass.fbo->bind();
									}
									break;
									case Pass::Level::BrightnessAssignmentMap:
									{
										auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::BrightnessAssignmentMap");
										const auto & worldParameters = world->getParameters();
										const auto & featherSize = worldParameters.getFloat("Feather size").get();
										const auto & targetBrightness = worldParameters.getFloat("Target brightness").get();
										shader.begin();
										{
											shader.setUniformTexture("AvailabilitySelf", this->passes[Pass::Level::AvailabilityProjection].fbo->getTexture(), 0);
											shader.setUniformTexture("AvailabilityAll", this->passes[Pass::Level::AccumulateAvailability].fbo->getTexture(), 1);
											shader.setUniform2f("TextureResolution", ofVec2f(width, height));
											shader.setUniform1f("FeatherSize", featherSize);
											shader.setUniform1f("TargetBrightness", targetBrightness);

											plane.draw();
										}
										shader.end();
									}
									break;
									default:
										break;
									}
								}
								pass.fbo->end();

								pass.renderedFrameIndex = currentFrameIndex;
							}
						}
					}
				}
			}

			//---------
			float Projector::getBrightness() const {
				return this->parameters.brightness;
			}

			//---------
			void Projector::exportPass(Pass::Level passLevel, string filename /*= ""*/) {
				if (filename.empty()) {
					auto passString = Pass::toString(passLevel);
					auto result = ofSystemSaveDialog(passString + ".exr", "Save " + passString);
					if (!result.bSuccess) {
						throw(ofxRulr::Exception("No valid filename selected"));
					}
					filename = result.filePath;
				}

				//read the data from the pass
				auto & pass = this->getPass(passLevel, true);
				ofFloatPixels floatPixels;
				pass.fbo->getTexture().readToPixels(floatPixels);
				
				auto extension = ofFilePath::getFileExt(filename);
				if (extension == "exr" || extension == "hdr") {
					//save the float data directly into these formats
					ofSaveImage(floatPixels, filename);
				}
				else {
					//convert to 8bit for other formats
					ofPixels pixels;
					pixels.allocate(floatPixels.getWidth()
						, floatPixels.getHeight()
						, floatPixels.getPixelFormat());

					auto size = (size_t) floatPixels.size();
					auto input = floatPixels.getData();
					auto output = pixels.getData();
					for (size_t i = 0; i < size; i++) {
						float value = *input * 255.0f;
						*output = value > 255.0f ? 255.0f : value;
						input++;
						output++;
					}

					ofSaveImage(pixels, filename);
				}
			}

			//---------
			void Projector::renderAvailability(shared_ptr<Projector> projector, shared_ptr<World> world, ofxRay::Camera & view, shared_ptr<Nodes::Base> scene) {
				auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::AvailabilityProjection");
				
				//Get constants for shader
				const auto & worldParameters = world->getParameters();
				const auto & normalCutoffAngle = worldParameters.getFloat("Normal cutoff angle (°)").get();
				const auto & featherSize = worldParameters.getFloat("Feather size").get();

				auto viewOtherProjectorNode = projector->getInput<Item::Projector>();
				if (!viewOtherProjectorNode) {
					return;
				}

				view.beginAsCamera();
				{
					ofxRulr::Utils::Graphics::glEnable(GL_DEPTH_TEST);
					{
						shader.begin();
						{
							shader.setUniform1f("normalCutoff", cos(normalCutoffAngle * DEG_TO_RAD));
							shader.setUniform1f("featherSize", featherSize);

							auto viewOtherProjector = viewOtherProjectorNode->getViewInWorldSpace();
							shader.setUniformMatrix4f("projectorVP0"
								, viewOtherProjector.getViewMatrix() * viewOtherProjector.getClippedProjectionMatrix());
							shader.setUniform2f("projectorResolution0", ofVec2f(viewOtherProjector.getWidth(), viewOtherProjector.getHeight()));
							shader.setUniform3f("projectorPosition0", view.getPosition());
							shader.setUniform1f("projectorBrightness0", projector->getBrightness());

							{
								auto & depthTexture = projector->getPass(Pass::Level::Color, false).fbo->getDepthTexture();
								shader.setUniformTexture("projectorDepthTexture0", depthTexture, 0);
								shader.setUniformTexture("projectorDepthTextureShadow0", depthTexture, 1);
							}

							DrawWorldAdvancedArgs drawArgs;
							{
								drawArgs.enableMaterials = false;
								drawArgs.enableColors = false;
								drawArgs.enableNormals = true;
								drawArgs.enableStyle = false;
								drawArgs.enableTextures = false;
							}

							scene->drawWorldAdvanced(drawArgs);
						}
						shader.end();
					}
					ofxRulr::Utils::Graphics::glDisable(GL_DEPTH_TEST);
				}
				view.endAsCamera();
			}
		}
	}
}
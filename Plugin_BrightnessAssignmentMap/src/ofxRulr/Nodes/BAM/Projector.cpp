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
						oneChannel3D.internalformat = GL_R32F;
						oneChannel3D.depthStencilInternalFormat = GL_DEPTH_COMPONENT32;
					}
					
					this->passes = Passes{
						{ Pass::Level::Color, color3D },
						//{ Pass::Level::Depth, depth3D },
						{ Pass::Level::DepthPreview, color2D },
						{ Pass::Level::AvailabilityProjection, oneChannel3D },
						{ Pass::Level::AccumulateAvailability, oneChannel2D }
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
						}
					}
				}
				this->manageParameters(this->parameters);				

				this->panelGroup->onDraw += [this](ofxCvGui::DrawArguments &) {

				};
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

						ofxRulr::Utils::Graphics::glPushAttrib(GL_POLYGON_BIT);
						ofxRulr::Utils::Graphics::glDisable(GL_CULL_FACE);
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

									if (i == Pass::Level::Color) {
										this->panelGroup->add(ofxCvGui::Panels::makeTexture(pass.fbo->getDepthTexture(), "Depth RAW"));
									}
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
										view.beginAsCamera(true);
										{
											ofxRulr::Utils::Graphics::glEnable(GL_DEPTH_TEST);
											{
												ofxRulr::DrawWorldAdvancedArgs args;
												args.enableGui = false;
												args.enableStyle = false;
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
										view.beginAsCamera(true);
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
											depthTexture.unbind(0);
										}
										shader.end();
										break;
									}
									case Pass::Level::AvailabilityProjection:
									{
										ofClear(0, 0);
									}
									break;
									case Pass::Level::AccumulateAvailability:
									{
										pass.fbo->unbind();
										{
											//temporarily unbind the fbo since we need to draw into the fbo for one projector
											auto allProjectors = world->getProjectors();
											const auto & accumulateFbo = this->passes[Pass::Level::AvailabilityProjection].fbo;
											auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::AvailabilityProjection");

											//setup parameters from world
											{
												const auto & worldParameters = world->getParameters();
												const auto normalCutoffAngle = worldParameters.getFloat("Normal cutoff angle (°)").get();
												const auto featherSize = worldParameters.getFloat("Feather size").get();
												shader.setUniform1f("normalCutoff", cos(normalCutoffAngle * DEG_TO_RAD));
												shader.setUniform1f("featherSize", featherSize);
											}

											for (auto otherProjector : allProjectors) {
												accumulateFbo->begin();
												{
													ofClear(0, 0);
													view.beginAsCamera(true);
													{
														shader.begin();
														{
															auto viewOtherProjectorNode = otherProjector->getInput<Item::Projector>();
															if (!viewOtherProjectorNode) {
																continue;
															}

															auto viewOtherProjector = viewOtherProjectorNode->getViewInWorldSpace();
															shader.setUniformMatrix4f("projectorVP0"
																, viewOtherProjector.getViewMatrix() * viewOtherProjector.getClippedProjectionMatrix());
															shader.setUniform2f("projectorResolution0", ofVec2f(viewOtherProjector.getWidth(), viewOtherProjector.getHeight()));
															shader.setUniform3f("projectorPosition0", view.getPosition());
															shader.setUniform1f("projectorBrightness0", otherProjector->getBrightness());

															{
																auto & depthTexture = otherProjector->getPass(Pass::Level::Color, false).fbo->getDepthTexture();
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
													view.endAsCamera();
												}
												accumulateFbo->end();

												pass.fbo->begin();
												{
													ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
													{
														accumulateFbo->draw(0, 0);
													}
													ofDisableBlendMode();
												}
												pass.fbo->end();
											}

										}
										pass.fbo->bind();
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
		}
	}
}
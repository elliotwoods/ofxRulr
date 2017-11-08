#include "pch_Plugin_BrightnessAssignmentMap.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			//----------
			PreviewCoverage::PreviewCoverage() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string PreviewCoverage::getTypeName() const {
				return "BAM::PreviewCoverage";
			}

			//----------
			void PreviewCoverage::init() {
				RULR_NODE_DRAW_WORLD_LISTENER;
				this->addInput<World>();
			}

			//----------
			void PreviewCoverage::drawWorldStage() {
				auto world = this->getInput<World>();
				if (world) {
					const auto & worldParameters = world->getParameters();
					const auto & normalCutoffAngle = worldParameters.getFloat("Normal cutoff angle (°)").get();
					const auto & featherSize = worldParameters.getFloat("Feather size").get();

					auto projectors = world->getProjectors();
					if (!projectors.empty()) {
						auto scene = world->getInput<Nodes::Base>("Scene");
						if (scene) {
							//before running the shader, let's get all the passes ready
							for (auto projector : projectors) {
								projector->render(Pass::Level::Done);
							}

							auto & shader = ofxAssets::shader("ofxRulr::Nodes::BAM::PreviewCoverage");
							shader.begin();
							{
								shader.setUniform1f("normalCutoff", cos(normalCutoffAngle * DEG_TO_RAD));
								shader.setUniform1f("featherSize", featherSize);

								int projectorCount = 0;
								for (auto projector : projectors) {
									auto projectorIndexString = ofToString(projectorCount);

									{
										auto viewProjector = projector->getInput<Item::Projector>();
										auto view = viewProjector->getViewInWorldSpace();
										shader.setUniformMatrix4f("projectorVP" + projectorIndexString
											, view.getViewMatrix() * view.getClippedProjectionMatrix());
										shader.setUniform2f("projectorResolution" + projectorIndexString, ofVec2f(view.getWidth(), view.getHeight()));
										shader.setUniform3f("projectorPosition" + projectorIndexString, view.getPosition());
										shader.setUniform1f("projectorBrightness" + projectorIndexString, projector->getBrightness());
									}

									{
										auto & depthTexture = projector->getPass(Pass::Level::Color, false).fbo->getDepthTexture();
										shader.setUniformTexture("projectorDepthTexture" + projectorIndexString, depthTexture, projectorCount * 2 + 0);
										shader.setUniformTexture("projectorDepthTextureShadow" + projectorIndexString, depthTexture, projectorCount * 2 + 1);
									}

									projectorCount++;
								}
								shader.setUniform1i("projectorCount", projectorCount);

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
					}
				}
			}
		}
	}
}
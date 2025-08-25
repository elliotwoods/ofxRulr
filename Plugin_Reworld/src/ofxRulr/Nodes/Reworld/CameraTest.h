#pragma once

#include "ofxRulr.h"

#include "Router.h"

// Note we've hardcoded the mask resolution as 512

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			class CameraTest: public Nodes::Base {
			public:
				MAKE_ENUM(BlendMode
					, (Alpha, Add)
					, ("Alpha", "Add"));

				MAKE_ENUM(IterationMode
					, (Polar, Cartesian)
					, ("Polar", "Cartesian"));

				MAKE_ENUM(State
					, (Ready, Poll, Running, Finish, Fail)
					, ("Ready", "Poll", "Running", "Finish", "Fail"));

				MAKE_ENUM(CaptureTarget
					, (Direct, Image)
					, ("Direct", "Image"));

				struct Capture {
					glm::vec2 prismPosition;
					glm::vec2 scanAreaPosition;
					cv::Mat image;
					ofTexture preview;
					void updatePreview() {
						this->preview.loadData(this->image.data
							, this->image.cols
							, this->image.rows
							, GL_RGB);
					}
				};

				CameraTest();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&) const;
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;

				glm::vec2 positionToScanArea(const glm::vec2&);
				glm::vec2 scanAreaToPosition(const glm::vec2&);
				static glm::vec2 positionToPolar(const glm::vec2&);
				static glm::vec2 polarToPosition(const glm::vec2&);
				static glm::vec2 polarToAxes(const glm::vec2&);
				static glm::vec2 axesToPolar(const glm::vec2&);

				Router::Address getTargetAddress() const;

				void clearCaptures();
			protected:
				void calculateIterations();
				void calculateIterationsPolar();
				void calculateIterationsCartesian();
				void removeIterationsCloseToCaptures();
				void sortIterationsByDistance();
				void calculateScanAreaPositions();
				
				void startScanRoutine();
				void updateScanRoutine();

				ofxCvGui::PanelPtr panel;

				struct : ofParameterGroup {
					ofParameter<State> state {"State", State::Ready};

					struct : ofParameterGroup {
						ofParameter<int> column{ "Column", 0, 0, 255 };
						ofParameter<int> portal{ "Portal", 9, 1, 127 };
						PARAM_DECLARE("Target", column, portal);
					} target;

					struct : ofParameterGroup {
						ofParameter<bool> locked{ "Locked", false };

						ofParameter<float> opacity{ "Opacity", 0.5f, 0.0f, 1.0f };
						ofParameter<BlendMode> blendMode{ "Blend mode", BlendMode::Alpha };

						ofParameter<float> radius{ "Radius", 128, 1, 256 };

						PARAM_DECLARE("Mask", locked, opacity, blendMode, radius);
					} mask;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> moveTimeout{ "Move timeout", 20.0f };
							ofParameter<float> pollFrequency{ "Poll frequency", 0.5f };
							ofParameter<float> epsilon{ "Epsilon", 0.0001, 0, 1.0f };
							PARAM_DECLARE("Movements", moveTimeout, pollFrequency, epsilon);
						} movements;
						
						struct : ofParameterGroup {
							ofParameter<IterationMode> mode{ "Iteration mode", IterationMode::Cartesian };
							ofParameter<float> minDistanceToExisting{ "Min distance to existing", 0.03f };
							ofParameter<bool> alwaysCalculate{ "Always calculate", true };

							struct : ofParameterGroup {
								ofParameter<float> minR{ "Min R", 0.01, 0, 1 };
								ofParameter<float> maxR{ "Max R", 1, 0, 1 };
								ofParameter<int> rSteps{ "R steps", 64 };
								ofParameter<float> minTheta{ "Min theta", 0, 0, TWO_PI };
								ofParameter<float> maxTheta{ "Max theta", TWO_PI, 0, TWO_PI };
								ofParameter<int> thetaSteps{ "Theta steps", 64 };
								ofParameter<bool> scaleThetaStepsWithR{ "Scale theta steps with R", true };
								PARAM_DECLARE("Polar", minR, maxR, rSteps, minTheta, maxTheta, thetaSteps, scaleThetaStepsWithR);
							} polar;

							struct : ofParameterGroup {
								ofParameter<float> maxR{ "Max R", 0.8, 0, 1 };
								ofParameter<int> steps{ "Steps", 9, 1, 64 };
								PARAM_DECLARE("Cartesian", maxR, steps);
							} cartesian;

							PARAM_DECLARE("Iterations", mode, minDistanceToExisting, alwaysCalculate, polar, cartesian);
						} iterations;

						ofParameter<CaptureTarget> captureTarget{ "Capture target", CaptureTarget::Image };

						PARAM_DECLARE("Capture", movements, iterations, captureTarget);
					} capture;

					struct : ofParameterGroup {
						ofParameter<int> resolution{ "Resolution", 2048 };
						ofParameter<float> stampSize{ "Stamp size", 128, 1, 512};
						PARAM_DECLARE("Scan area", resolution, stampSize);
					} scanArea;

					PARAM_DECLARE("CameraTest", state, target, mask, capture, scanArea);
				} parameters;

				ofImage preview;
				cv::Mat image;
				bool isFrameNew = false;
				vector<glm::vec2> sourceCorners;
				vector<glm::vec2> targetCorners;
				int selectedCornerIndex = 0;

				glm::mat4 homography;
				bool transformDirty = true;

				struct {
					cv::Mat squareImage;
					ofImage preview;
				} captureRegion;

				struct {
					cv::Mat image;
					ofImage preview;
					bool previewDirty = false;

					// Normalised x,y coordinates [-1..1]
					vector<glm::vec2> iterationPositionsPrism;

					// image space coordinates to match scanArea.image
					vector<glm::vec2> iterationPositionsScanArea;

					ofPolyline linePreview;
				} scanArea;

				struct {
					chrono::system_clock::time_point lastPoll = chrono::system_clock::now();
					size_t currentIndex = 0;
					glm::vec2 currentPosition;
				} scanRoutine;

				struct {
					glm::vec2 position;
					glm::vec2 targetPosition;
				} currentState;

				vector<Capture> captures;
			};
		}
	}
}
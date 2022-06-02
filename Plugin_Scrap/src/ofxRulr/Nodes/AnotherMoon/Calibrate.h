#pragma once

#include "ofxRulr.h"
#include "ofxCeres.h"
#include "ofxRulr/Solvers/LineToImage.h"
#include "ofxRulr/Solvers/LineMCToImage.h"
#include "ofxRulr/Solvers/LinesWithCommonPoint.h"

#include "Lasers.h"

/// <summary>
/// Class for handling the ">>" to drill down into this selection
/// </summary>
/// <typeparam name="T"></typeparam>
template<typename T>
struct EditSelection
{
	T* selection = nullptr;
	ofxLiquidEvent<void> onSelectionChanged;

	bool isSelected(const T* const item) {
		return item == this->selection;
	}
	void select(T* const item) {
		if (item != this->selection) {
			this->selection = item;
			this->onSelectionChanged.notifyListeners();
		}
	}
	void deselect() {
		this->select(nullptr);
	}
};

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			class Calibrate : public Nodes::Base {
			public:
				MAKE_ENUM(LaserState
					, (Shutdown, Standby, Run, TestPattern)
					, ("Shutdown", "Standby", "Run", "TestPattern"));

				MAKE_ENUM(ImageFileSource
					, (Camera, Local)
					, ("Camera", "Local"));

				struct ImagePath {
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);
					string pathOnCamera;
					filesystem::path localCopy;
				};

				struct DrawParameters : ofParameterGroup {
					struct CameraCaptures : ofParameterGroup {
						ofParameter<WhenActive> cameras{ "Cameras", WhenActive::Always };
						PARAM_DECLARE("Camera captures", cameras);
					} cameraCaptures;

					struct BeamCaptures : ofParameterGroup {
						ofParameter<WhenActive> rays{ "Rays", WhenActive::Never };
						ofParameter<WhenActive> rayIndices{ "Ray indices", WhenActive::Never };
						PARAM_DECLARE("Beam captures", rays, rayIndices);
					} beamCaptures;

					PARAM_DECLARE("Draw", cameraCaptures, beamCaptures)
				};

				struct DrawArguments {
					const Nodes::Base* nodeForSelection;
					shared_ptr<Item::Camera> camera;
					shared_ptr<Lasers> lasers;
					const DrawParameters & drawParameters;
				};

				class BeamCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					BeamCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					glm::vec2 projectionPoint;

					ImagePath onImage;
					ImagePath offImage;

					Models::Line line;

					EditSelection<BeamCapture>* parentSelection = nullptr;
					float residual = 0.0f;

					/// <summary>
					/// Denotes if the projectionPoint is offset by the lasers' centerOffset when captured.
					/// </summary>
					bool isOffset = true;
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
				};


				class LaserCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					LaserCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					void drawWorldStage(const DrawArguments&);

					int laserAddress;
					Utils::CaptureSet<BeamCapture> beamCaptures;
					filesystem::path directory;

					glm::vec2 imagePointInCamera;

					EditSelection<BeamCapture> ourSelection;
					EditSelection<LaserCapture>* parentSelection = nullptr;

					cv::Mat preview;

					struct {
						bool success = false;
						float residual = 0.0f;
					} linesWithCommonPointSolveResult;

					struct : ofParameterGroup {
						ofParameter<bool> markBad{ "Mark bad", false };
						ofParameter<bool> useAlternativeSolve{ "Use alternative solve", false };
						PARAM_DECLARE("LaserCapture", markBad, useAlternativeSolve);
					} parameters;

				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
				};


				class CameraCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					CameraCapture();
					string getName() const;
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					void update();
					void drawWorldStage(const DrawArguments&);

					Utils::CaptureSet<LaserCapture> laserCaptures;
					filesystem::path directory;

					EditSelection<LaserCapture> ourSelection;
					EditSelection<CameraCapture>* parentSelection = nullptr;

					shared_ptr<Item::RigidBody> cameraTransform = make_shared<Item::RigidBody>();
				protected:
					void drawObjectSpace(const DrawArguments&);
					ofxCvGui::ElementPtr getDataDisplay() override;
				};

				Calibrate();
				string getTypeName() const override;

				void init();
				void update();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;
				void drawWorldStage();

				void capture();

				void deselectLasersWithNoData(size_t minimumCameraCaptureCount = 1);
				void offsetBeamCaptures();

				void calibrateLines(); // Note that process is in seperate Calibrate_Process.cpp
				void calibrateInitialCameras();
				void calibrateBundleAdjustPoints();
				void calibrateBundleAdjustLasers(bool performSolve); // Otherwise just get the residuals

				void pruneBeamCapturesByResidual(float);

				EditSelection<CameraCapture> cameraEditSelection;

				const Utils::CaptureSet<CameraCapture> & getCameraCaptures();
				filesystem::path getLocalCopyPath(const ImagePath& cameraPath) const;
			protected:
				struct SolverSettings : ofParameterGroup {
					ofParameter<bool> printOutput{ "Print output", true };
					ofParameter<int> maxIterations{ "Max iterations", 1000 };
					ofParameter<int> threads{ "Threads", 16 };
					ofParameter<float> functionTolerance{ "Function tolerance", 1e-6, 0, 1 };
					ofParameter<float> parameterTolerance{ "Parameter tolerance", 1e-8, 0, 1 };
					PARAM_DECLARE("Solver settings", printOutput, maxIterations, threads, functionTolerance, parameterTolerance);
				};

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> radius{ "Radius", 0.5, 0, 1.0 };
							ofParameter<int> resolution{ "Resolution", 3, 2, 4 };
							PARAM_DECLARE("Image points"
								, radius
								, resolution);
						} imagePoints;

						struct : ofParameterGroup {
							ofParameter<string> hostname{ "Hostname", "10.0.0.180" };
							ofParameter<float> captureTimeout{ "Capture timeout [s]", 10.0f };
							PARAM_DECLARE("Remote camera"
								, hostname
								, captureTimeout);
						} remoteCamera;

						ofParameter<LaserState> laserStateForOthers{ "State for others", LaserState::Standby };
						ofParameter<float> outputDelay{ "Output delay [s]", 0.5, 0, 5.0 };
						ofParameter<bool> continueOnFail{ "Continue on fail", false };
						ofParameter<int> messageTransmitTries{ "Messaege transmit tries", 5 };

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							PARAM_DECLARE("Dry run", enabled);
						} dryRun;

						PARAM_DECLARE("Capture"
							, imagePoints
							, remoteCamera
							, laserStateForOthers
							, outputDelay
							, continueOnFail
							, messageTransmitTries
							, dryRun);
					} capture;

					struct : ofParameterGroup {
						ofParameter<ImageFileSource> imageFileSource{ "Image file source", ImageFileSource::Local };
						ofParameter<filesystem::path> localDirectory{ "Local directory", "E:\\DCIM\\100EOS5D" };
						ofParameter<bool> amplifyBlue{ "Amplify blue", false };
						ofParameter<float> differenceThreshold{ "Difference threshold", 4, 0, 255 };
						ofParameter<float> normalizePercentile{ "Normalize percentile", 1e-5 };
						ofParameter<float> distanceThreshold{ "Distance threshold [px]", 10, 0, 1000 };
						ofParameter<float> minMeanPixelValueOnLine{ "Min mean pixel value on line", 32, 0, 255 };

						SolverSettings solverSettings;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true};
							ofParameter<bool> popup{ "Popup", false };
							ofParameter<bool> save{ "Save", true };
							PARAM_DECLARE("Preview", enabled, popup, save);
						} preview;

						PARAM_DECLARE("Line finder"
							, imageFileSource
							, localDirectory
							, amplifyBlue
							, differenceThreshold
							, normalizePercentile
							, distanceThreshold
							, minMeanPixelValueOnLine
							, solverSettings
							, preview);
					} lineFinder;

					struct : ofParameterGroup {
						ofParameter<bool> useExtrinsicGuess{ "Use extrinsic guess", false };
						PARAM_DECLARE("Initial cameras", useExtrinsicGuess);
					} initialCameras;

					struct : ofParameterGroup {
						ofParameter<bool> sceneCenterConstraint{ "Scene center contraint", true };
						ofParameter<bool> sceneRadiusConstraint{ "Scene radius contraint", true };

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							ofParameter<int> cameraIndex{ "Camera index", 0};
							PARAM_DECLARE("Camera with 0 yaw", enabled, cameraIndex);
						} cameraWith0Yaw;

						ofParameter<bool> planeConstraint{"Plane constraint", true};

						struct : ofParameterGroup {
							struct : ofParameterGroup {
								ofParameter<bool> position{ "Position", false };
								ofParameter<bool> rotation{ "Rotation", false };
								ofParameter<bool> fov{ "FOV", false };
								PARAM_DECLARE("Lasers", position, rotation, fov);
							} lasers;

							ofParameter<bool> cameras{ "Cameras", false };

							PARAM_DECLARE("Fixed", lasers, cameras)
						} fixed;

						SolverSettings solverSettings;

						PARAM_DECLARE("Bundle adjustment"
							, sceneCenterConstraint
							, sceneRadiusConstraint
							, cameraWith0Yaw
							, planeConstraint
							, fixed
							, solverSettings);
					} bundleAdjustment;

					DrawParameters draw;

					PARAM_DECLARE("Calibrate", capture, lineFinder, initialCameras, bundleAdjustment, draw);
				} parameters;

				vector<glm::vec2> getCalibrationImagePoints() const;
				void waitForDelay() const;
				string getBaseCameraURL() const;

				string captureToURL();

				void takePhoto();
				vector<string> pollNewCameraFiles();
				cv::Mat fetchImage(const ImagePath&) const;
				cv::Mat fetchImageAmplifyBlue(const ImagePath&) const;
				void configureSolverSettings(ofxCeres::SolverSettings&, const SolverSettings& parameters) const;

				void selectAllChildren();
				void selectChildrenWithSolvedLines();
				void deselectChildrenWithBadLines();


				ofxCvGui::PanelPtr panel;

				Utils::CaptureSet<CameraCapture> cameraCaptures;
				ofURLFileLoader urlLoader;

			};
		}
	}
}
#pragma once

#include "ofxRulr.h"
#include "ofxCeres.h"
#include "ofxRulr/Solvers/LineToImage.h"
#include "ofxRulr/Solvers/LineMCToImage.h"
#include "ofxRulr/Solvers/LinesFromPoint.h"

#define LINEMODEL Solvers::LineToImage

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
					, (Off, TestPattern)
					, ("Off", "TestPattern"));

				MAKE_ENUM(ImageFileSource
					, (Camera, Local)
					, ("Camera", "Local"));

				class BeamCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					BeamCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					glm::vec2 imagePoint;
					string urlOnImage;
					string urlOffImage;

					Solvers::LineToImage::Line line;

					EditSelection<BeamCapture>* parentSelection = nullptr;
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

					int laserAddress;
					Utils::CaptureSet<BeamCapture> beamCaptures;

					glm::vec2 imagePointInCamera;

					EditSelection<BeamCapture> ourSelection;
					EditSelection<LaserCapture>* parentSelection = nullptr;

					cv::Mat preview;
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
				};

				class CameraCapture : public Utils::AbstractCaptureSet::BaseCapture
				{
				public:
					CameraCapture();
					string getDisplayString() const override;
					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);

					Utils::CaptureSet<LaserCapture> laserCaptures;

					EditSelection<LaserCapture> ourSelection;
					EditSelection<CameraCapture>* parentSelection = nullptr;
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
				};

				Calibrate();
				string getTypeName() const override;

				void init();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;

				void capture();
				void process();

				EditSelection<CameraCapture> cameraEditSelection;
			protected:
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

						ofParameter<LaserState> laserStateForOthers{ "State for others", LaserState::TestPattern };
						ofParameter<float> outputDelay{ "Output delay [s]", 0.5, 0, 5.0 };
						ofParameter<bool> continueOnFail{ "Continue on fail", false };
						ofParameter<int> signalSends{ "Signal sends", 10 };

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
							, signalSends
							, dryRun);
					} capture;

					struct : ofParameterGroup {
						ofParameter<ImageFileSource> imageFileSource{ "Image file source", ImageFileSource::Local };
						ofParameter<string> localPath{ "Local path", "E:\\DCIM\\100EOS5D\\0M8A4113.JPG" };
						ofParameter<float> differenceThreshold{ "Difference threshold", 32, 0, 255 };
						ofParameter<float> normalizePercentile{ "Normalize percentile", 1e-5 };
						ofParameter<float> distanceThreshold{ "Distance threshold [px]", 10, 0, 1000 };
						ofParameter<float> minMeanPixelValueOnLine{ "Min mean pixel value on line", 10, 0, 255 };

						PARAM_DECLARE("Processing"
							, imageFileSource
							, localPath
							, differenceThreshold
							, normalizePercentile
							, distanceThreshold
							, minMeanPixelValueOnLine);
					} processing;

					struct : ofParameterGroup {
						ofParameter<bool> printOutput{ "Print output", true };
						ofParameter<int> maxIterations{ "Max iterations", 1000 };
						ofParameter<int> threads{ "Threads", 16 };
						PARAM_DECLARE("Solver", printOutput, maxIterations, threads);
					} solver;

					PARAM_DECLARE("Calibrate", capture, processing, solver);
				} parameters;

				vector<glm::vec2> getCalibrationImagePoints() const;
				void waitForDelay() const;
				string getBaseCameraURL() const;

				string captureToURL();

				void takePhoto();
				vector<string> pollNewCameraFiles();
				cv::Mat fetchImage(const string& cameraPath) const;
				void configureSolverSettings(ofxCeres::SolverSettings&) const;

				ofxCvGui::PanelPtr panel;

				Utils::CaptureSet<CameraCapture> cameraCaptures;
				ofURLFileLoader urlLoader;
			};
		}
	}
}
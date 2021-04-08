#pragma once

#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class ProjectorToPlanes : public Nodes::Base {
			public:
				struct ProjectorPixel {
					glm::vec3 world;
					glm::vec2 projection;
				};

				ProjectorToPlanes();
				string getTypeName() const override;
				void init();

				ofxCvGui::PanelPtr getPanel() override;

				void drawWorldStage();
				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				void calibrate();
				void selectProjectorPixels();
				void projectorPixelPositions();
				void solveProjector();
			protected:
				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<float> minimumThreshold{ "Minimum threshold", 3, 0, 255 };
						ofParameter<float> ratio{ "Ratio", 2, 1, 10 };
						ofParameter<int> apertureSize{ "Aperture Size", 3, 1, 11};
						ofParameter<int> dilations{ "Dilations", 20, 0, 10 };
						ofParameter<float> threshold{ "Threshold", 1, 0, 255 };
						PARAM_DECLARE("Edges", minimumThreshold, ratio, apertureSize, dilations, threshold);
					} edges;

					struct : ofParameterGroup {
						ofParameter<bool> customUndistort{ "Custom undistort", true };
						PARAM_DECLARE("Project pixels", customUndistort);
					} projectPixels;

					struct : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<float> lensOffset{ "Lens offset", 0, -1.0, 1.0 };
							ofParameter<float> throwRatio{ "Throw ratio", 1, 0, 5 };
							PARAM_DECLARE("Initial", lensOffset, throwRatio);
						} initial;

						ofParameter<int> decimateData{ "Decimate data", 100 };
						ofParameter<bool> filterOutliers{ "Filter outliers", true };
						ofParameter<float> maximumReprojectionError{ "Max reprojection error", 5.0, 0.0, 100.0 };

						PARAM_DECLARE("Calibrate", initial, decimateData, filterOutliers, maximumReprojectionError);
					} calibrate;

					PARAM_DECLARE("ProjectorToPlanes", edges, projectPixels, calibrate)
				} parameters;

				struct {
					cv::Mat edges;
					cv::Mat pixelsOnPlane;

					vector<ProjectorPixel> projectorPixels;

					float reprojectionError;
				} data;

				struct {
					ofImage edges;
					ofImage pixelsOnPlane;
					ofMesh projectorPixelsMesh;
				} preview;

				ofxCvGui::PanelPtr panel;
			};
		}
	}
}
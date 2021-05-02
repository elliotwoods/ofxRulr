#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"
#include <aruco/aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {
				class HaloBoard : public Nodes::Item::AbstractBoard {
				public:
					HaloBoard();
					string getTypeName() const override;
					void init();
					void update();
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					void populateInspector(ofxCvGui::InspectArguments &);

					bool findBoard(cv::Mat
						, vector<cv::Point2f> & imagePoints
						, vector<cv::Point3f> & objectPoints
						, FindBoardMode findBoardMode
						, cv::Mat cameraMatrix
						, cv::Mat distortionCoefficients) const override;

					void drawObject() const override;
					float getSpacing() const override;
					glm::vec3 getCenter() const;

					ofxCvGui::PanelPtr getPanel() override;

					glm::vec2 getPhysicalSize() const;

					vector<int> getNonMirroringIDs() const;

				protected:
					bool findBoard(cv::Mat
						, vector<cv::Point2f>& imagePoints
						, vector<cv::Point3f>& objectPoints) const;

					float getPreviewPixelsPerMeter() const;

					struct Parameters : ofParameterGroup {
						struct : ofParameterGroup {
							ofParameter<int> width{ "Width", 5 };
							ofParameter<int> height{ "Height", 5 };
							PARAM_DECLARE("Size [squares]", width, height);
						} size;

						struct : ofParameterGroup {
							ofParameter<float> square{ "Square", 0.036, 0.001, 0.10 };
							ofParameter<float> outerCorner{ "Outer corner", 0.01, 0, 0.1 };
							PARAM_DECLARE("Length [m]", square, outerCorner);
						} length;

						struct : ofParameterGroup {
							ofParameter<int> DPI{ "Preview DPI", 75 };
							PARAM_DECLARE("Preview", DPI);
						} preview;

						struct : ofParameterGroup {
							ofParameter<int> openCorners{ "Open corners [px]", 0 };

							struct : ofParameterGroup {
								ofParameter<bool> enclosedMarkers{ "Enclosed markers", true };
								ofParameter<float> borderDistanceThreshold{ "Border distance threshold", 0.015f };
								PARAM_DECLARE("Params", enclosedMarkers, borderDistanceThreshold);
							} params;
							PARAM_DECLARE("Detection", openCorners, params);
						} detection;

						struct : ofParameterGroup {
							ofParameter<bool> enabled{ "Enabled", true };
							PARAM_DECLARE("Refinement", enabled);
						} refinement;

						PARAM_DECLARE("HaloBoard", size, length, preview, detection, refinement);
					};

					struct Board {
						struct Position {
							int x;
							int y;

							bool operator==(const Position & other) const {
								return other.x == this->x && other.y == this->y;
							}
						};

						struct Marker {
							int id;
							Position position;
						};

						struct MarkerCorner {
							int markerID;
							int cornerIndex;

							bool operator==(const MarkerCorner & other) const {
								return other.markerID == this->markerID && other.cornerIndex == this->cornerIndex;
							}
						};

						struct BoardCorner {
							Position position;
							vector<MarkerCorner> markerCorners;
						};

						map<int, Marker> markers;
						vector<BoardCorner> corners;
					};

					Parameters parameters;
					Parameters cachedParameters;
					
					aruco::Dictionary dictionary;
					shared_ptr<aruco::MarkerDetector> detector;
					float cachedDPI = 0;

					shared_ptr<Board> board;
					ofImage preview;

					ofxCvGui::PanelPtr panel;
				};
			}
		}
	}
}
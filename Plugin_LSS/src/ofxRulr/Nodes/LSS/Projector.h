#pragma once

#include "pch_Plugin_LSS.h"
#include "ofxMessagePack.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			class Projector : public Nodes::Base {
			public:
				struct ProjectorPixelFind {
					ofxRay::Ray cameraPixelRay;
					glm::vec2 cameraPixelXY;
					glm::vec2 cameraPixelXYUndistorted;
					MSGPACK_DEFINE(cameraPixelRay.s.x, cameraPixelRay.s.y, cameraPixelRay.s.z, cameraPixelRay.t.x, cameraPixelRay.t.y, cameraPixelRay.t.z
						, cameraPixelXY.x, cameraPixelXY.y
						, cameraPixelXYUndistorted.x, cameraPixelXYUndistorted.y);
				};

				class Scan : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Scan();
					string getDisplayString() const override;
					std::string getTypeName() const override;
					void deserialize(const nlohmann::json &);
					void serialize(nlohmann::json &);
					string getFilename() const;
					void drawWorld();

					ofParameter<glm::vec3> cameraPosition{ "Camera position", glm::vec3() };
					map<uint32_t, ProjectorPixelFind> projectorPixels;
				protected:
					ofMesh preview;
					void rebuildPreview();
					bool previewDirty = true;
				};

				struct vec3s : glm::vec3 {
					vec3s operator=(const glm::vec3 & vec) {
						*(glm::vec3*) this = vec;
						return *this;
					}
					MSGPACK_DEFINE(x, y, z);
				};

				struct vec2s : glm::vec2 {
					MSGPACK_DEFINE(x, y);
				};

				struct Vertex {
					glm::vec3 world;
					size_t projector; // pixel coordinate
					glm::vec2 projectorNormalizedXY;
					MSGPACK_DEFINE(world.x
						, world.y
						, world.z
						, projector
						, projectorNormalizedXY.x
						, projectorNormalizedXY.y);
				};

				struct Line {
					Line() {
						this->color = ofColor(200, 100, 100);
						static int colorIndex = 0;
						colorIndex++;
						this->color.setHueAngle((colorIndex * 10) % 360);
					}
					void drawWorld(bool labels) const;

					vector<Vertex> vertices;
					ofColor color;
					ofVbo vbo;

					int lineIndex;
					int projectorIndex;
					vec2s startProjector;
					vec2s endProjector;

					vec3s startWorld;
					vec3s endWorld;

					string lastUpdate;
					string lastEditBy;
					double age;

					MSGPACK_DEFINE(
						vertices
						, color.r, color.g, color.b
						, lineIndex
						, projectorIndex
						, startProjector, endProjector
						, startWorld, endWorld
						, lastUpdate, lastEditBy, age);
				};

				struct LineSearchParams : ofParameterGroup {
					ofParameter<float> headSize{ "Head size", 0.05f };
					ofParameter<float> trunkThickness{ "Trunk thickness", 0.01f };
					ofParameter<float> initialInclusionThreshold{ "Inclusion threshold", 0.8f };
					ofParameter<int> minimumCount{ "Minimum count", 10};
					PARAM_DECLARE("Line search", headSize, trunkThickness, initialInclusionThreshold, minimumCount);
				};

				Projector();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				void deserialize(const nlohmann::json &);
				void serialize(nlohmann::json &);
				void populateInspector(ofxCvGui::InspectArguments &);

				void addScan(shared_ptr<Scan>);
				void triangulate(float maxResidual);
				void autoMapping(const LineSearchParams & params);

				void loadMapping(const string & filename);
				void dipLinesInData(); 
				void calibrateProjector();
				void trimLinesExceptOne(int lineIndex);

				ofxCvGui::PanelPtr getPanel() override;

				vector<Line> & getLines();
				int getProjectorIndex() const;
			protected:
				void rebuildPreviews();

				Utils::CaptureSet<Scan> scans;
				ofxCvGui::PanelPtr panel;
				
				vector<Vertex> unclassifiedVertices;
				ofVbo unclassifiedVerticesPreview;
				ofImage projectorSpacePreview;

				bool previewsDirty = true;

				struct : ofParameterGroup {
					ofParameter<int> projectorIndex{ "Projector Index", 0 }; 

					struct : ofParameterGroup {
						ofParameter<WhenDrawOnWorldStage> rays{ "Draw rays", WhenDrawOnWorldStage::Selected };
						ofParameter<WhenDrawOnWorldStage> unclassifiedVertices{ "Draw unclassified vertices", WhenDrawOnWorldStage::Always };
						ofParameter<WhenDrawOnWorldStage> lines{ "Lines", WhenDrawOnWorldStage::Always };
						ofParameter<WhenDrawOnWorldStage> lineLabels{ "Line labels", WhenDrawOnWorldStage::Selected };
						ofParameter<bool> linesOnProjectorPreview{ "Lines on projector preview", true };
						PARAM_DECLARE("Draw", rays, unclassifiedVertices, lines, lineLabels, linesOnProjectorPreview);
					} draw;
					
					ofParameter<float> maximumResidual{ "Maximum residual", 0.05, 0.0001, 1.0};
					LineSearchParams lineSearch;

					struct : ofParameterGroup {
						ofParameter<float> searchThickness{ "Search thickness", 10.0f };
						ofParameter<bool> progressive{ "Progressive", true };
						PARAM_DECLARE("Data dip", searchThickness, progressive);
					} dataDip;

					PARAM_DECLARE("Projector"
						, projectorIndex
						, draw
						, maximumResidual
						, lineSearch
						, dataDip);
				} parameters;

				vector<Line> lines;
			};
		}
	}
}
#pragma once

#include "ofxRulr.h"
#include "ofxRulr/Nodes/Item/RigidBody.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Caustics {
			class Target : public ofxRulr::Nodes::Item::RigidBody {
			public:
				class Curve {
				public:
					struct DrawArguments {
						bool showInfo = false;
					};

					void serialize(nlohmann::json&) const;
					void deserialize(const nlohmann::json&);

					void clear();
					void addVertex(const glm::vec3&);
					void addVertices(const vector<glm::vec3>&);

					float getLength() const;
					const vector<glm::vec3>& getVertices() const;
					vector<glm::vec3>& getVerticesForWriting();

					void resample(size_t);

					void calc();
					void draw(const DrawArguments& = DrawArguments()) const;
				protected:
					
					vector<glm::vec3> vertices;

					map<float, glm::vec3> verticesByPosition;
					float length;
					
					struct {
						ofPolyline line;
					} preview;
				};

				Target();
				string getTypeName() const override;
				void init();
				void update();
				ofxCvGui::PanelPtr getPanel();

				void drawObject();
				
				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				void clearDrawing();
				void normaliseDrawing();

				vector<Curve> getCurves() const;
				vector<Curve> getResampledCurves(size_t) const;
				void resampleDrawing(size_t size);

				std::vector<glm::vec3> getTargetPoints() const;
				std::vector<glm::vec3> getTargetPointsForCurves(const vector<Curve>&) const;

			protected:
				struct : ofParameterGroup {
					ofParameter<float> scale{ "Scale", 1.0f, 0.0f, 10.0f };
					ofParameter<bool> locked{ "Locked", false };

					struct : ofParameterGroup {
						ofParameter<bool> drawInfo{ "Draw info", false };
						PARAM_DECLARE("Debug", drawInfo);
					} debug;
					PARAM_DECLARE("Target", scale, locked, debug);
				} parameters;

				vector<Curve> curves;
				Curve newCurve;
			};
		}
	}
}
#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Utils/CaptureSet.h"
#include "ofxNonLinearFit.h"
#include "MarkerMap.h"

#include <aruco/aruco.h>

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			class PLUGIN_ARUCO_EXPORTS AlignMarkerMap : public Nodes::Base {
			public:
				enum class Plane : int {
					X, Y, Z
				};

				enum class Points : int {
					All, Center
				};

				class Constraint : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Constraint();
					string getDisplayString() const override;

					ofParameter<int> markerID{ "Marker ID", 0 };
					ofParameter<int> plane{ "Plane", (int)Plane::X };
					ofParameter<float> offset{ "Offset", 0.0f };
					ofParameter<int> points{ "Points", (int)Points::All };
					ofParameter<float> residual{ "Residual (m)", 0.0f };

					float getResidual(const ofMatrix4x4 & transform);
					void updatePoints(shared_ptr<aruco::MarkerMap>);
					void drawWorld();
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
					void serialize(nlohmann::json &);
					void deserialize(const nlohmann::json &);
					vector<ofVec3f> cachedPoints;
				};

				class Model : public ofxNonLinearFit::Models::Base<shared_ptr<Constraint>, Model> {
				public:
					unsigned int getParameterCount() const override;
					void getResidual(shared_ptr<Constraint>, double & residual, double * gradient) const override;
					void evaluate(shared_ptr<Constraint> &) const override;
					void cacheModel() override;
					ofMatrix4x4 getTransform() const;
				protected:
					ofMatrix4x4 cachedModel;
				};

				AlignMarkerMap();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();
				ofxCvGui::PanelPtr getPanel() override;

				void populateInspector(ofxCvGui::InspectArguments &);
				void serialize(nlohmann::json &);
				void deserialize(const nlohmann::json &);

				void fit();
			protected:
				Utils::CaptureSet<Constraint> constraints;
				ofxCvGui::PanelPtr panel;

				struct : ofParameterGroup {
					ofParameter<float> residual{ "Residual", 0.0f };
					PARAM_DECLARE("AlignMarkerMap", residual);
				} parameters;
			};
		}
	}
}
#pragma once

#include "Constants_Plugin_ArUco.h"
#include "ofxRulr/Nodes/Base.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include <aruco/aruco.h>
#include "ofxRulr/Utils/CaptureSet.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MarkerMap {
			class PLUGIN_ARUCO_EXPORTS Markers : public Nodes::Base {
			public:
				class Marker : public Utils::AbstractCaptureSet::BaseCapture, public ofxCvGui::IInspectable, public enable_shared_from_this<Marker> {
				public:
					Marker();
					string getDisplayString() const override;
					string getTypeName() const override;

					void setParent(Markers*);
					void drawWorld();

					void serialize(nlohmann::json&);
					void deserialize(const nlohmann::json&);
					void populateInspector(ofxCvGui::InspectArguments&);

					vector<glm::vec3> getObjectVertices() const;
					vector<glm::vec3> getWorldVertices() const;

					struct : ofParameterGroup {
						ofParameter<int> ID{ "ID", 1 };
						ofParameter<bool> fixed{ "Fixed", false };
						ofParameter<float> length{ "Length [m]", 0.16 };
						ofParameter<bool> ignore{ "Ignore", false };
						PARAM_DECLARE("Marker", ID, fixed, length, ignore);
					} parameters;

					shared_ptr<Nodes::Item::RigidBody> rigidBody;
				protected:
					ofxCvGui::ElementPtr getDataDisplay() override;
					Markers* markers = nullptr;
				};

				Markers();
				string getTypeName() const override;
				void init();
				void update();
				void drawWorldStage();

				void populateInspector(ofxCvGui::InspectArguments&);
				void serialize(nlohmann::json&);
				void deserialize(const nlohmann::json&);

				ofxCvGui::PanelPtr getPanel() override;

				const ofImage & getMarkerImage(int ID) const;

				vector<shared_ptr<Marker>> getMarkers() const;
				shared_ptr<Marker> getMarkerByID(int) const;

				void add();
				void add(shared_ptr<Marker>);

				void sort();

				void deleteUnfixedMarkers();
			protected:
				shared_ptr<ofxCvGui::Panels::Widgets> panel;
				Utils::CaptureSet<Marker> markers;

				struct : ofParameterGroup {
					struct : ofParameterGroup {
						ofParameter<WhenActive> labels{ "Labels", WhenActive::Selected };
						ofParameter<WhenActive> outlines{ "Outlines", WhenActive::Always };
						PARAM_DECLARE("Draw", labels, outlines);
					} draw;

					PARAM_DECLARE("Markers", draw);
				} parameters;
			};
		}
	}
}
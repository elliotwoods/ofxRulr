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
					ofVec2f cameraPixelXY;
					MSGPACK_DEFINE(cameraPixelRay.s.x, cameraPixelRay.s.y, cameraPixelRay.s.z, cameraPixelRay.t.x, cameraPixelRay.t.y, cameraPixelRay.t.z);
				};

				class Scan : public Utils::AbstractCaptureSet::BaseCapture {
				public:
					Scan();
					string getDisplayString() const override;
					std::string getTypeName() const override;
					void deserialize(const Json::Value &);
					void serialize(Json::Value &);
					string getFilename() const;
					void drawWorld();

					ofParameter<ofVec3f> cameraPosition{ "Camera position", ofVec3f() };
					map<uint32_t, ProjectorPixelFind> projectorPixels;
				protected:
					ofMesh preview;
					void rebuildPreview();
					bool previewDirty = true;
				};

				Projector();
				string getTypeName() const override;
				void init();
				void drawWorldStage();

				void deserialize(const Json::Value &);
				void serialize(Json::Value &);
				void populateInspector(ofxCvGui::InspectArguments &);

				void addScan(shared_ptr<Scan>);
				void triangulate();
				ofxCvGui::PanelPtr getPanel() override;
			protected:
				Utils::CaptureSet<Scan> scans;
				ofxCvGui::PanelPtr panel;
				ofMesh triangulatedMesh;

				struct : ofParameterGroup {
					ofParameter<WhenDrawOnWorldStage> drawRays{ "Draw rays", WhenDrawOnWorldStage::Selected };
					ofParameter<WhenDrawOnWorldStage> drawVertices{ "Draw vertices", WhenDrawOnWorldStage::Always };
					PARAM_DECLARE("Projector", drawRays, drawVertices);
				} parameters;
			};
		}
	}
}
#pragma once

#include "ofxRulr/Nodes/Base.h"
#include "ofxCvGui/Panels/Groups/Strip.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				namespace Orbbec {
					class ColorRegistration : public Nodes::Base {
					public:
						struct Capture {
							vector<ofVec2f> irImagePoints;
							vector<ofVec2f> colorImagePoints;
							vector<ofVec3f> boardPoints;
						};

						ColorRegistration();
						string getTypeName() const;

						void init();
						void update();
						void populateInspector(ofxCvGui::InspectArguments &);
						void serialize(Json::Value &);
						void deserialize(const Json::Value &);

						void addCapture();
						void calibrate();

						ofxCvGui::PanelPtr getPanel() override;

						ofVec3f cameraToDepth(const ofVec2f & camera);
						ofVec3f cameraToWorld(const ofVec2f & camera);
					protected:
						struct : ofParameterGroup {
							ofParameter<float> irExposure{ "IR exposure", 0.15, 0, 8 };
							ofParameter<bool> calibrateIRIntrinsics{ "Calibrate IR Intrinsics", false };
							ofParameter<bool> allowDistortion{ "Allow distortion", false };
							ofParameter<bool> renderDepthMap{ "Render depth map", false };
							PARAM_DECLARE("ColorRegistration", irExposure, calibrateIRIntrinsics, allowDistortion, renderDepthMap)
						} parameters;

						ofParameter<float> reprojectionError{ "Reprojection error", 0 };

						void renderDepthMap();

						shared_ptr<ofxCvGui::Panels::Groups::Strip> panelStrip;

						vector<Capture> captures;
						ofTexture colorPreview;
						ofTexture irPreview;

						ofFbo drawPointCloud;
						ofFbo extractDepthBuffer;
						ofFloatPixels colorDepthBufferPixels;

						struct {
							ofVec3f world;
							ofVec2f camera;
						} testPixel;
					};
				}
			}
		}
	}
}
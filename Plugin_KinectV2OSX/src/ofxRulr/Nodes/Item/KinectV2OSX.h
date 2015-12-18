#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxMultiKinectV2.h"

namespace ofxRulr {
    namespace Nodes {
        namespace Item {
            class KinectV2OSX : public IDepthCamera {
            public:
                KinectV2OSX();
                ~KinectV2OSX();
                
                string getTypeName() const override;
                ofxCvGui::PanelPtr getView() override;
                
                void init();
                void update();
                
                void drawObject() override;
                
                ofPixels * getColorPixels() override;
                ofFloatPixels * getDepthPixelsFloat() override;
                ofFloatPixels * getWorldPixels() override;
            protected:
                ofxCvGui::PanelPtr view;
                shared_ptr<ofxMultiKinectV2> kinect;

                ofFloatPixels worldPixels;
                ofTexture colorPreview;
                ofTexture depthPreview;
                ofVboMesh pointCloud;
            };
        }
    }
}
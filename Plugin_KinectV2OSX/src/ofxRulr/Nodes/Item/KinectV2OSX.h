#pragma once

#include "ofxRulr/Nodes/Item/IDepthCamera.h"
#include "ofxMultiKinectV2.h"
#include "GpuRegistration.h"

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
				ofPixels * getIRPixels() override;
				ofShortPixels * getIRPixelsShort() override;
                ofFloatPixels * getDepthPixelsFloat() override;
                ofFloatPixels * getWorldPixels() override;
				
				ofTexture * getIRTexture() override;
				
				ofImage * getColorInDepthImage() override;
            protected:
                ofxCvGui::PanelPtr view;
                shared_ptr<ofxMultiKinectV2> kinect;
				shared_ptr<GpuRegistration> gpuRegistration;
				
				ofShortImage irImage;
				ofPixels irPixels8;
                ofFloatPixels worldPixels;
				ofImage colorInDepth;
				
                ofTexture colorPreview;
                ofTexture depthPreview;
                ofVboMesh pointCloud;
            };
        }
    }
}
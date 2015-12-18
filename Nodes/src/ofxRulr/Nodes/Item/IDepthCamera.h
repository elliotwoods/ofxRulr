#pragma once

#include "RigidBody.h"
#include "ofPixels.h"

namespace ofxRulr {
    namespace Nodes {
        namespace Item {
            class IDepthCamera : public Item::RigidBody {
            public:
                virtual string getTypeName() const override {
                    return "Item::IDepthCamera";
                }
                
                virtual ofPixels * getColorPixels() {
                    return nullptr;
                }
                
                virtual ofShortPixels * getDepthPixelsShort() {
                    return nullptr;
                }
                
                virtual ofFloatPixels * getDepthPixelsFloat() {
                    return nullptr;
                }
                
                virtual ofFloatPixels * getWorldPixels() {
                    return nullptr;
                }
            };
        }
    }
}
#include "KinectV2OSX.h"
#include "ofxCvGui/Panels/Groups/Grid.h"
#include "ofxCvGui/Panels/Draws.h"

using namespace ofxCvGui;

namespace ofxRulr {
    namespace Nodes {
        namespace Item {
            //----------
            KinectV2OSX::KinectV2OSX() {
                RULR_NODE_INIT_LISTENER;
            }
            
            //----------
            KinectV2OSX::~KinectV2OSX() {
                if(this->kinect) {
                    this->kinect->close();
                    this->kinect.reset();                    
                }
            }
            
            //----------
            string KinectV2OSX::getTypeName() const {
                return "Item::KinectV2OSX";
            }
            
            //----------
            PanelPtr KinectV2OSX::getView() {
                return this->view;
            }
            
            //----------
            void KinectV2OSX::init() {
                RULR_NODE_UPDATE_LISTENER;
                
                this->kinect = make_shared<ofxMultiKinectV2>();
                
                this->kinect->open();
                this->kinect->start();
                
                
                //--
                //VIEW
                //--
                //
                auto view = make_shared<Panels::Groups::Grid>();
                
                auto colorPanel = make_shared<Panels::Draws>(this->colorPreview);
                colorPanel->setCaption("Color");
                view->add(colorPanel);
                
                auto depthPanel = make_shared<Panels::Draws>(this->depthPreview);
                depthPanel->setCaption("Depth");
                view->add(depthPanel);
                
                this->view = view;
                //
                //--
                
                
                this->pointCloud.setUsage(GL_DYNAMIC_DRAW);
                this->pointCloud.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);
            }
            
            //----------
            void KinectV2OSX::update() {
                this->kinect->update();
                if(this->kinect->isFrameNew()) {
                    const auto & colorPixels = this->kinect->getColorPixelsRef();
                    const auto & depthPixels = this->kinect->getDepthPixelsRef();

                    if(colorPixels.isAllocated()) {
                        this->colorPreview.loadData(colorPixels);
                    }
                    
                    if (depthPixels.isAllocated()) {
                        this->depthPreview.loadData(depthPixels);
                        
                        if(this->worldPixels.getWidth() != depthPixels.getWidth() || this->worldPixels.getHeight() != depthPixels.getHeight()) {
                            this->worldPixels.allocate(depthPixels.getWidth(), depthPixels.getHeight(), 3);
                        }
                        
                        const auto depthWidth = depthPixels.getWidth();
                        const auto depthHeight = depthPixels.getHeight();
                        auto worldVectors = (ofVec3f *) worldPixels.getData();
#pragma omp parallel for
                        for(int j=0; j<depthHeight; j++) {
							const auto pixelOffset = depthWidth * j;
							auto worldVectorsRow = worldVectors + pixelOffset;
							for(int i=0; i<depthWidth; i++) {
								worldVectorsRow[i] = this->kinect->getWorldCoordinateAt(i, j, depthPixels[i + pixelOffset]);
							}
						}
                    }
                    
                    //adapted from ofxMultiKinectV2 example_pointcloud
                    if(this->worldPixels.isAllocated()) {
						const auto size = this->worldPixels.size();
						if(this->pointCloud.getNumVertices() != size) {
							//reallocate
							this->pointCloud.getVertices().resize(size);
							this->pointCloud.getColors().resize(size);
						}
						
						//copy world data in
						memcpy(this->pointCloud.getVerticesPointer(), this->worldPixels.getData(), size * sizeof(ofVec3f));
						//force ofMesh to mark as dirty
						this->pointCloud.setVertex(0, this->pointCloud.getVertex(0));
                    }
                }
            }
            
            //----------
            void KinectV2OSX::drawObject() {
                ofPushMatrix();
                {
                    const auto scaleFactor = 0.001f;
                    ofScale(scaleFactor, scaleFactor, scaleFactor); //mm to m
                    this->pointCloud.draw();
                }
                ofPopMatrix();
            }
			
			//----------
			ofPixels * KinectV2OSX::getColorPixels() {
				return & this->kinect->getColorPixelsRef();
			}
			
			//----------
			ofFloatPixels * KinectV2OSX::getDepthPixelsFloat() {
				return & this->kinect->getDepthPixelsRef();
			}
			
			//----------
			ofFloatPixels * KinectV2OSX::getWorldPixels() {
				return & this->worldPixels;
			}
        }
    }
}
#include "KinectV2OSX.h"
#include "ofxCvGui/Panels/Groups/Grid.h"
#include "ofxCvGui/Panels/Draws.h"

#include "ofxCvGui/Widgets/EditableValue.h"

#include "GpuRegistration.h"

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
				this->closeDevice();
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
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				
				this->deviceIndex.set("Device index", 0);
				
				this->openDevice();
                
                this->pointCloud.setUsage(GL_DYNAMIC_DRAW);
                this->pointCloud.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);
            }
            
            //----------
            void KinectV2OSX::update() {
				if(this->kinect) {
					this->kinect->update();
					if(this->kinect->isFrameNew()) {
						const auto & colorPixels = this->kinect->getColorPixelsRef();
						const auto & depthPixels = this->kinect->getDepthPixelsRef();
						const auto & irPixels = this->kinect->getIrPixelsRef();
						
						if(colorPixels.isAllocated()) {
							this->colorPreview.loadData(colorPixels);
						}
						
						if (depthPixels.isAllocated()) {
							this->depthPreview.loadData(depthPixels);
							
							//needed for registration
							this->depthPreview.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
							
							if(this->worldPixels.getWidth() != depthPixels.getWidth() || this->worldPixels.getHeight() != depthPixels.getHeight()) {
								this->worldPixels.allocate(depthPixels.getWidth(), depthPixels.getHeight(), 3);
							}
							
							//--
							//build world pixels
							//--
							//
							const auto depthWidth = depthPixels.getWidth();
							const auto depthHeight = depthPixels.getHeight();
							auto worldVectors = (ofVec3f *) worldPixels.getData();
#pragma omp parallel for
							for(int j=0; j<depthHeight; j++) {
								const auto pixelOffset = depthWidth * j;
								auto worldVectorsRow = worldVectors + pixelOffset;
								for(int i=0; i<depthWidth; i++) {
									worldVectorsRow[i] = this->kinect->getWorldCoordinateAt(i, j, depthPixels[i + pixelOffset]) / 1000.0f;
								}
							}
							//
							//--
							
							
							//--
							//build color in depth pixels
							//--
							//
							this->gpuRegistration->update(this->depthPreview, this->colorPreview, false);
							auto & registeredTexture = this->gpuRegistration->getRegisteredTexture();
							registeredTexture.readToPixels(this->colorInDepth.getPixels());
							this->colorInDepth.update();
							//
							//--
						}
						
						if(irPixels.isAllocated()) {
							this->irImage.allocate(irPixels.getWidth(), irPixels.getHeight(), OF_IMAGE_GRAYSCALE);
							const auto size = irPixels.size();
							auto outPixels = this->irImage.getPixels().getData();
							for(int i=0; i<size; i++) {
								outPixels[i] = (uint16_t) irPixels[i];
							}
							this->irImage.update();
						}
						
						
						//--
						//build point cloud
						//--
						//
						if(this->worldPixels.isAllocated()) {
							const auto size = this->worldPixels.size();
							if(this->pointCloud.getNumVertices() != size) {
								this->pointCloud.getVertices().resize(size);
								this->pointCloud.getTexCoords().clear();
								const auto depthWidth = depthPixels.getWidth();
								const auto depthHeight = depthPixels.getHeight();
								for(int j=0; j<depthHeight; j++) {
									for(int i=0; i<depthWidth; i++) {
										this->pointCloud.addTexCoord(ofVec2f(i, j));
									}
								}
							}
							
							//copy world data in
							memcpy(this->pointCloud.getVerticesPointer(), this->worldPixels.getData(), size * sizeof(ofVec3f));
							//force ofMesh to mark as dirty
							this->pointCloud.setVertex(0, this->pointCloud.getVertex(0));
						}
						//
						//--
					}
				}
            }
			
			
			//----------
			void KinectV2OSX::serialize(Json::Value & json) {
				Utils::Serializable::serialize(this->deviceIndex, json);
			}
			
			//----------
			void KinectV2OSX::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(this->deviceIndex, json);
			}
			
			//----------
			void KinectV2OSX::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				auto deviceIndexWidget = Widgets::EditableValue<int>::make(this->deviceIndex);
				deviceIndexWidget->onEditValue += [this](string deviceIndexString) {
					this->deviceIndex = ofToInt(deviceIndexString);
					this->openDevice();
				};
				inspector->add(deviceIndexWidget);
			}
			
            //----------
            void KinectV2OSX::drawObject() {
				this->colorInDepth.bind();
				{
					glPushAttrib(GL_POINT_BIT);
					{
						glPointSize(3.0f);
						glEnable(GL_POINT_SMOOTH);
						this->pointCloud.draw();
					}
					glPopAttrib();
				}
				this->colorInDepth.unbind();
            }
			
			//----------
			ofPixels * KinectV2OSX::getColorPixels() {
				return & this->kinect->getColorPixelsRef();
			}
			
			//----------
			ofPixels * KinectV2OSX::getIRPixels() {
				this->irPixels8.allocate(this->irImage.getWidth(), this->irImage.getHeight(), 1);
				const auto irImageSize = this->irPixels8.size();
				auto irPixels = this->irPixels8.getData();
				auto irPixelsShort = this->irImage.getPixels().getData();
				for(int i=0; i<irImageSize; i++) {
					irPixels[i] = irPixelsShort[i] >> 8;
				}
				return & this->irPixels8;
			}
			
			//----------
			ofShortPixels * KinectV2OSX::getIRPixelsShort() {
				return & this->irImage.getPixels();
			}
			
			//----------
			ofFloatPixels * KinectV2OSX::getDepthPixelsFloat() {
				return & this->kinect->getDepthPixelsRef();
			}
			
			//----------
			ofFloatPixels * KinectV2OSX::getWorldPixels() {
				return & this->worldPixels;
			}
			
			//----------
			ofTexture * KinectV2OSX::getIRTexture() {
				return & this->irImage.getTexture();
			}
			
			//----------
			ofImage * KinectV2OSX::getColorInDepthImage() {
				return & this->colorInDepth;
			}
			
			//----------
			void KinectV2OSX::openDevice() {
				this->closeDevice();
				
				this->kinect = make_shared<ofxMultiKinectV2>();
				this->kinect->open(true, true, this->deviceIndex);
				this->kinect->start();
				
				this->gpuRegistration = make_shared<GpuRegistration>();
				this->gpuRegistration->setup(this->kinect->getProtonect(), 1.0f);
				
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
				
				auto irPanel = make_shared<Panels::Draws>(this->irImage);
				irPanel->setCaption("IR");
				view->add(irPanel);
				
				view->setColsCount(3);
				
				this->view = view;
				//
				//--
			}
			
			//----------
			void KinectV2OSX::closeDevice() {
				if(this->gpuRegistration) {
					this->gpuRegistration.reset();
				}
				if(this->kinect) {
					this->kinect->close();
					this->kinect.reset();
				}
				
				if(this->view) {
					this->view->clear();
				}
			}
        }
    }
}
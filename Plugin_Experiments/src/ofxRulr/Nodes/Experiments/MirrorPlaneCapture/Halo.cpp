#include "pch_Plugin_Experiments.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Experiments {
			namespace MirrorPlaneCapture {				
				//---------
				Halo::Halo() {
					RULR_NODE_INIT_LISTENER;
				}

				//---------
				string Halo::getTypeName() const {
					return "Halo::Halo";
				}

				//---------
				void Halo::init() {
					RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

					this->manageParameters(this->parameters);
				}

				//---------
				void Halo::drawObject() {
					const auto& resolution = this->parameters.draw.resolution.get();
					const auto radius = this->parameters.diameter.get() / 2.0f;
					const auto thickness = this->parameters.draw.thickness.get();

					if (this->parameters.draw.asPath.get()) {
						ofPolyline polyline;
						
						// Inside
						for (int i = 0; i <= resolution; i++) {
							auto theta = (float)i / (float)resolution * TWO_PI;
							polyline.addVertex({
								cos(theta) * (radius - thickness / 2.0f)
								, sin(theta) * (radius - thickness / 2.0f)
								, 0.0f
								});
						}

						// Outside
						for (int i = resolution; i >= 0; i--) {
							auto theta = (float)i / (float)resolution * TWO_PI;
							polyline.addVertex({
								cos(theta) * (radius + thickness / 2.0f)
								, sin(theta) * (radius + thickness / 2.0f)
								, 0.0f
								});
						}

						polyline.close();
						polyline.draw();
					}
					else {
						ofPushStyle();
						{
							ofNoFill();
							ofSetCircleResolution(this->parameters.draw.resolution.get());
							ofDrawCircle({ 0, 0, 0 }, radius);
						}
						ofPopStyle();
					}
				}
				
				//---------
				float Halo::getRadius() const {
					return this->parameters.diameter.get() / 2.0f;
				}
			}
		}
	}
}
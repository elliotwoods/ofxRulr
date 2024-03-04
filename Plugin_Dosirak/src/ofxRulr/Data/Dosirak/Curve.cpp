#include "pch_Plugin_Dosirak.h"
#include "Curve.h"

namespace ofxRulr {
	namespace Data {
		namespace Dosirak {
			//----------
			void
				Curve::updatePreview()
			{
				this->preview.clear();
				this->preview.addVertices(this->points);
				if (this->closed) {
					this->preview.close();
				}
			}

			//----------
			void
				Curve::drawPreview() const
			{
				ofPushStyle();
				{
					ofSetColor(this->color);
					if (this->points.size() == 0) {

					}
					else if (this->points.size() == 1) {
						ofDrawSphere(this->points.front(), 0.1f);
					}
					else {
						this->preview.draw();
					}
				}
				ofPopStyle();
			}
		}
	}
}
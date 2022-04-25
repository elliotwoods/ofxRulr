#pragma once

#include "Serializable.h"
#include "ofxCvGui/Element.h"
#include "ofxCvGui/InspectController.h"

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Utils {
		class OFXRULR_API_ENTRY LambdaDrawable : public ofBaseDraws {
		public:
			typedef function<void(const ofRectangle&)> DrawAction;

			LambdaDrawable(const DrawAction&
				, float defaultWidth = 100
				, float defaultHeight = 100);

			void draw(float x, float y, float w, float h) const override;
			void draw(const ofRectangle& rect) const override;

			float getWidth() const override;
			float getHeight() const override;
		protected:
			const DrawAction drawAction;
			const ofRectangle defaultBounds;
		};
	}
}

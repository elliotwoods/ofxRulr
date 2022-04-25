#include "pch_RulrCore.h"

#include "LambdaDrawable.h"

namespace ofxRulr {
	namespace Utils {
		//----------
		LambdaDrawable::LambdaDrawable(const DrawAction& drawAction
			, float defaultWidth
			, float defaultHeight)
			: drawAction(drawAction)
			, defaultBounds(0, 0, defaultWidth, defaultHeight)
		{
		}

		//----------
		void
			LambdaDrawable::draw(float x, float y, float w, float h) const
		{
			this->drawAction({ x, y, w, h });
		}

		//----------
		void
			LambdaDrawable::draw(const ofRectangle& bounds) const
		{
			this->drawAction(bounds);
		}

		//----------
		float
			LambdaDrawable::getWidth() const
		{
			return this->defaultBounds.width;
		}

		//----------
		float 
			LambdaDrawable::getHeight() const
		{
			return this->defaultBounds.height;
		}
	}
}
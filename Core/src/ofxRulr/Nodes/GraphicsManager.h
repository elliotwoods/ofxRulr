#pragma once

#include "ofxSingleton.h"

#include "ofColor.h"
#include "ofImage.h"
#include "ofxAssets.h"

#include "ofxRulr/Utils/Constants.h"

namespace ofxRulr {
	namespace Nodes {
		class OFXRULR_API_ENTRY GraphicsManager : public ofxSingleton::Singleton<GraphicsManager> {
		public:
			GraphicsManager();

			shared_ptr<ofxAssets::Image> getIcon(const string & nodeTypeName);
			void setIcon(const string & nodeTypeName, shared_ptr<ofxAssets::Image>);

			ofColor getColor(const string & nodeTypeName);
			void setColor(const string & nodeTypeName, const ofColor &);
		protected:
			map<string, shared_ptr<ofxAssets::Image>> cachedIcons;
			map<string, ofColor> colors;
		};
	}
}
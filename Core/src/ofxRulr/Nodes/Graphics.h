#pragma once

#include "ofxSingleton.h"

#include "ofColor.h"
#include "ofImage.h"
#include "ofxAssets.h"

namespace ofxRulr {
	namespace Nodes {
		class Graphics : public ofxSingleton::Singleton<Graphics> {
		public:
			Graphics();
			shared_ptr<ofImage> getIcon(const string & nodeTypeName);
			ofColor getColor(const string & nodeTypeName);

			void setIcon(const string & nodeTypeName, shared_ptr<ofImage>);
			void setColor(const string & nodeTypeName, const ofColor &);
		protected:
			map<string, shared_ptr<ofImage>> icons;
			map<string, ofColor> colors;
		};
	}
}
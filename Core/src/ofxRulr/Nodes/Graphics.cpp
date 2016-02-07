#include "pch_RulrCore.h"
#include "Graphics.h"

//----------
OFXSINGLETON_DEFINE(ofxRulr::Nodes::Graphics);

namespace ofxRulr {
	namespace Nodes {
		//----------
		Graphics::Graphics() {

		}

		//----------
		shared_ptr<ofImage> Graphics::getIcon(const string & nodeTypeName) {
			auto findIcon = this->icons.find(nodeTypeName);
			if (findIcon == this->icons.end()) {
				//no icon yet, let's load it
				auto imageName = "ofxRulr::Nodes::" + nodeTypeName;

				pair<string, shared_ptr<ofImage>> inserter;
				if (!ofxAssets::hasImage(imageName)) {
					//the image file doesn't exist, let's use default icon
					imageName = "ofxRulr::Nodes::Default";
				}
				auto image = make_shared<ofImage>();
				image->clone(ofxAssets::image(imageName));
				inserter.second = image;
				this->icons.insert(inserter);
				return inserter.second;
			}
			else {
				return findIcon->second;
			}
		}

		//----------
		ofColor Graphics::getColor(const string & nodeTypeName) {
			auto findColor = this->colors.find(nodeTypeName);
			if (findColor == this->colors.end()) {
				//no color yet, let's calculate it
				auto hash = std::hash<string>()(nodeTypeName);
				auto hue = hash % 256;
				auto saturation = (hash >> 8) % 128;
				auto brightness = (hash >> 12) % 64 + 192;

				pair<string, ofColor> inserter;
				inserter.second.setHsb(hue, saturation, brightness);
				this->colors.insert(inserter);

				return inserter.second;
			}
			else {
				return findColor->second;
			}
		}

		//----------
		void Graphics::setIcon(const string & nodeTypeName, shared_ptr<ofImage> icon) {
			this->icons[nodeTypeName] = icon;
		}

		//----------
		void Graphics::setColor(const string & nodeTypeName, const ofColor & color) {
			//see setIcon for notes
			auto & storedColor = this->colors[nodeTypeName];
			storedColor = color;
		}
	}
}
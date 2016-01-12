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
				if (!ofxAssets::Register::X().hasImage(imageName)) {
					//the image file doesn't exist, let's use default icon
					imageName = "ofxRulr::Nodes::Default";
				}
				inserter.second = ofxAssets::Register::X().getImagePointer(imageName);
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
		void Graphics::setIcon(const string & nodeTypeName, const ofImage & icon) {
			// change existing image instance rather than replacing it
			// so that existing references will also be updated
			auto & storedIcon = this->icons[nodeTypeName];
			storedIcon->clone(icon);
		}

		//----------
		void Graphics::setColor(const string & nodeTypeName, const ofColor & color) {
			//see setIcon for notes
			auto & storedColor = this->colors[nodeTypeName];
			storedColor = color;
		}
	}
}
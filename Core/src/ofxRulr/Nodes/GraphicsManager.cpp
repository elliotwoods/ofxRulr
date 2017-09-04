#include "pch_RulrCore.h"
#include "GraphicsManager.h"

//----------
OFXSINGLETON_DEFINE(ofxRulr::Nodes::GraphicsManager);

namespace ofxRulr {
	namespace Nodes {
		//----------
		GraphicsManager::GraphicsManager() {

		}

		//----------
		shared_ptr<ofxAssets::Image> GraphicsManager::getIcon(const string & nodeTypeName) {
			//see if it's in our cached set
			auto findIcon = this->cachedIcons.find(nodeTypeName);
			if (findIcon != this->cachedIcons.end()) {
				return findIcon->second;
			}

			//otherwise let's choose an icon for this guy

			//build the node name path
			list<string> nodeNamePath;
			{
				auto splitVector = ofSplitString(nodeTypeName, "::", true, true);
				nodeNamePath.insert(nodeNamePath.begin(), splitVector.begin(), splitVector.end());
				nodeNamePath.push_front("Nodes");
				nodeNamePath.push_front("ofxRulr");
			}

			const auto & imageAssetSet = ofxAssets::AssetRegister().getImages();

			auto pathToSting = [](const list<string> & path) {
				stringstream ss;
				for (const auto & branch : path) {
					ss << branch << "::";
				}
				auto text = ss.str();

				//remove the last ::
				text.pop_back();
				text.pop_back();

				return text;
			};

			shared_ptr<ofxAssets::Image> imageAsset;

			while (imageAssetSet.find(pathToSting(nodeNamePath)) == imageAssetSet.end()) {
				if (nodeNamePath.back() == "Base") {
					nodeNamePath.pop_back(); // remove last 'Base'

					if (nodeNamePath.empty()) {
						//no we're at the top
						nodeNamePath.push_back("ofxRulr");
						nodeNamePath.push_back("Nodes");
						nodeNamePath.push_back("Base");
						break; // let's serve this up
					}

					//if we're here then we go up one tier
					nodeNamePath.pop_back();
					nodeNamePath.push_back("Base");
				}
				else {
					//we're not looking at a Base, let's just try Base of current namespace
					nodeNamePath.pop_back();
					nodeNamePath.push_back("Base");
				}
			}
			imageAsset = imageAssetSet[pathToSting(nodeNamePath)];
			this->cachedIcons.emplace(nodeTypeName, imageAsset);
			return imageAsset;
		}

		//----------
		void GraphicsManager::setIcon(const string & nodeTypeName, shared_ptr<ofxAssets::Image> icon) {
			this->cachedIcons[nodeTypeName] = icon;
		}

		//----------
		ofColor GraphicsManager::getColor(const string & nodeTypeName) {
			auto findColor = this->colors.find(nodeTypeName);
			if (findColor == this->colors.end()) {
				auto color = ofxCvGui::Utils::toColor(nodeTypeName);
				this->colors.emplace(nodeTypeName, color);
				return color;
			}
			else {
				return findColor->second;
			}
		}

		//----------
		void GraphicsManager::setColor(const string & nodeTypeName, const ofColor & color) {
			//see setIcon for notes
			auto & storedColor = this->colors[nodeTypeName];
			storedColor = color;
		}
	}
}
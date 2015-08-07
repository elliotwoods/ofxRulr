#pragma once

#include "NodeHost.h"

namespace ofxRulr {
	namespace Graph {
		namespace Editor {
			class LinkHost : public ofxCvGui::Element {
			public:
				typedef unsigned int Index;
				LinkHost();
				bool isValid() const;
				void updateBounds(); //ideally can be called from listener instead of every frame
				ofColor getColor() const;
			protected:
				void callbackDraw(ofxCvGui::DrawArguments &);
				virtual ofVec2f getSourcePinPosition() const;
				virtual ofVec2f getTargetPinPosition() const;
				weak_ptr<NodeHost> sourceNode;
				weak_ptr<NodeHost> targetNode;
				weak_ptr<AbstractPin> targetPin;
			};

			class TemporaryLinkHost : public LinkHost {
			public:
				TemporaryLinkHost(shared_ptr<NodeHost>, shared_ptr<AbstractPin>);
				void setCursorPosition(const ofVec2f &);
				void setSource(shared_ptr<NodeHost>);

				///actually make the connection
				bool flushConnection();
			protected:
				ofVec2f getSourcePinPosition() const override;
				ofVec2f cursorPosition;
			};

			class ObservedLinkHost : public LinkHost {
			public:
				ObservedLinkHost(shared_ptr<NodeHost> sourceNode, shared_ptr<NodeHost> targetNode, shared_ptr<AbstractPin> targetPin);
			};
		}
	}
}

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
				void draw(ofxCvGui::DrawArguments&);
				void update();
				ofColor getColor() const;
			protected:
				void rebuild();
				virtual glm::vec2 getSourcePinPosition() const;
				virtual glm::vec2 getTargetPinPosition() const;
				weak_ptr<NodeHost> sourceNode;
				weak_ptr<NodeHost> targetNode;
				weak_ptr<AbstractPin> targetPin;

				bool needsRebuild = true;
				glm::vec2 cachedStart;
				glm::vec2 cachedEnd;
				ofPolyline polyline;
				ofVbo vbo;
			};

			class TemporaryLinkHost : public LinkHost {
			public:
				TemporaryLinkHost(shared_ptr<NodeHost>, shared_ptr<AbstractPin>);
				void setCursorPosition(const glm::vec2&);
				void setSource(shared_ptr<NodeHost>);

				///actually make the connection
				bool flushConnection();
			protected:
				glm::vec2 getSourcePinPosition() const override;
				glm::vec2 cursorPosition;
			};

			class ObservedLinkHost : public LinkHost {
			public:
				ObservedLinkHost(shared_ptr<NodeHost> sourceNode, shared_ptr<NodeHost> targetNode, shared_ptr<AbstractPin> targetPin);
			};
		}
	}
}

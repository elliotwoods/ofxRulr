#pragma once

#include "../Base.h"

#include "ofxCvGui/Panels/Image.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			class IReferenceVertices : public Procedure::Base {
			public:
				class Vertex {
				public:
					virtual ofVec3f getWorldPosition() const = 0;
					virtual void drawWorld(const ofColor & = ofColor(255));
				protected:
					void drawObjectLines();
				};

				IReferenceVertices();
				string getTypeName() const override;
				void drawWorld() override;

				const vector<shared_ptr<Vertex>> & getVertices() const;
			protected:
				vector<shared_ptr<Vertex>> vertices;
			};
		}
	}
}
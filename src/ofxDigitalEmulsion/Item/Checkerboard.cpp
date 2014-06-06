#include "Checkerboard.h"
#include "../../../addons/ofxCvGui2/src/ofxCvGui.h"
#include "ofxCvMin.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Checkerboard::Checkerboard() {
			this->sizeX.set("Size X", 9.0f, 2.0f, 20.0f);
			this->sizeY.set("Size Y", 5.0f, 2.0f, 20.0f);
			this->spacing.set("Spacing [m]", 0.025f, 0.001f, 1.0f);
		}

		//----------
		string Checkerboard::getTypeName() const {
			return "Checkerboard";
		}

		//----------
		void addSlider(ofParameter<float> & parameter, ElementGroupPtr inspector) {
			auto slider = Widgets::Slider::make(parameter);
			slider->addIntValidator();
			inspector->add(slider);
		}

		//----------
		void Checkerboard::populate(ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make("Checkerboard", Widgets::Title::Level::H2));

			addSlider(this->sizeX, inspector);
			addSlider(this->sizeY, inspector);
			addSlider(this->spacing, inspector);
		}

		//----------
		ofxCvGui::PanelPtr Checkerboard::getView() {
			auto view = MAKE(ofxCvGui::Panels::World);
			view->onDrawWorld += [this] (ofCamera &) {
				ofxCv::makeCheckerboardMesh(this->getSize(), this->spacing).drawFaces();
			};
			return view;
		}

		//----------
		cv::Size Checkerboard::getSize() {
			return cv::Size(this->sizeX, this->sizeY);
		}
	}
}
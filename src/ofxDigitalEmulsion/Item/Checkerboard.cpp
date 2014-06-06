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
			this->updatePreviewMesh();
		}

		//----------
		string Checkerboard::getTypeName() const {
			return "Checkerboard";
		}

		//----------
		shared_ptr<ofxCvGui::Widgets::Slider> addSlider(ofParameter<float> & parameter, ElementGroupPtr inspector) {
			auto slider = Widgets::Slider::make(parameter);
			slider->addIntValidator();
			inspector->add(slider);
			return slider;
		}

		//----------
		void Checkerboard::populateInspector(ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make("Checkerboard", Widgets::Title::Level::H2));

			auto sliderCallback = [this] (ofParameter<float> &) {
				this->updatePreviewMesh();
			};
			
			addSlider(this->sizeX, inspector)->onValueChange += sliderCallback;
			addSlider(this->sizeY, inspector)->onValueChange += sliderCallback;
			inspector->add(Widgets::Title::make("NB : The chessboard size is defined by the amount of inner corners", Widgets::Title::Level::H3));
			inspector->add(Widgets::Spacer::make());
			addSlider(this->spacing, inspector)->onValueChange += sliderCallback;
		}

		//----------
		ofxCvGui::PanelPtr Checkerboard::getView() {
			auto view = MAKE(ofxCvGui::Panels::World);
			view->onDrawWorld += [this] (ofCamera &) {
				this->previewMesh.draw();
			};
			view->setGridEnabled(false);
			view->getCamera().setCursorDraw(true, this->spacing / 5.0f);

			auto & camera = view->getCamera();
			auto distance = this->spacing * MAX(this->sizeX, this->sizeY);
			camera.setPosition(-distance, distance, -distance);
			camera.lookAt(ofVec3f());

			return view;
		}

		//----------
		cv::Size Checkerboard::getSize() {
			return cv::Size(this->sizeX, this->sizeY);
		}

		//----------
		void Checkerboard::updatePreviewMesh() {
			this->previewMesh = ofxCv::makeCheckerboardMesh(this->getSize(), this->spacing);
		}
	}
}
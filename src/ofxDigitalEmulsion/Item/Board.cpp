#include "Board.h"
#include "../../../addons/ofxCvGui2/src/ofxCvGui.h"
#include "ofxCvMin.h"
#include "../Utils/Gui.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Item {
		//----------
		Board::Board() {
			this->boardType.set("Board Type", 0, 0, 1);
			this->sizeX.set("Size X", 9.0f, 2.0f, 20.0f);
			this->sizeY.set("Size Y", 5.0f, 2.0f, 20.0f);
			this->spacing.set("Spacing [m]", 0.025f, 0.001f, 1.0f);
			this->updatePreviewMesh();
		}

		//----------
		string Board::getTypeName() const {
			return "Board";
		}

		//----------
		ofxCvGui::PanelPtr Board::getView() {
			auto view = MAKE(ofxCvGui::Panels::World);
			view->onDrawWorld += [this](ofCamera &) {
				this->previewMesh.draw();
			};
			view->setGridEnabled(false);
#ifdef USE_OFXGRABCAM
			view->getCamera().setCursorDraw(true, this->spacing / 5.0f);
#endif

			auto & camera = view->getCamera();
			auto distance = this->spacing * MAX(this->sizeX, this->sizeY);
			camera.setPosition(0, 0, -distance);
			camera.lookAt(ofVec3f(), ofVec3f(0, -1, 0));
			camera.setNearClip(distance / 100.0f);
			camera.setFarClip(distance * 100.0f);

			return view;
		}

		//----------
		void Board::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->boardType, json);
			Utils::Serializable::serialize(this->sizeX, json);
			Utils::Serializable::serialize(this->sizeY, json);
			Utils::Serializable::serialize(this->spacing, json);
		}

		//----------
		void Board::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->boardType, json);
			Utils::Serializable::deserialize(this->sizeX, json);
			Utils::Serializable::deserialize(this->sizeY, json);
			Utils::Serializable::deserialize(this->spacing, json);

			this->updatePreviewMesh();
		}

		//----------
		ofxCv::BoardType Board::getBoardType() const {
			switch (this->boardType) {
			case 0:
				return ofxCv::BoardType::Checkerboard;
			case 1:
				return ofxCv::BoardType::AsymmetricCircles;
			default:
				return ofxCv::BoardType::Checkerboard;
			}
		}

		//----------
		cv::Size Board::getSize() const {
			return cv::Size(this->sizeX, this->sizeY);
		}

		//----------
		vector<cv::Point3f> Board::getObjectPoints() const {
			return ofxCv::makeBoardPoints(this->getBoardType(), this->getSize(), this->spacing);
		}

		//----------
		bool Board::findBoard(cv::Mat image, vector<cv::Point2f> & results, bool useOptimisers) const {
			auto size = this->getSize();
			return ofxCv::findBoard(image, this->getBoardType(), size, results, useOptimisers);
		}

		//----------
		void Board::populateInspector2(ElementGroupPtr inspector) {
			auto sliderCallback = [this](ofParameter<float> &) {
				this->updatePreviewMesh();
			};

			auto typeChooser = Widgets::MultipleChoice::make("Board type");
			typeChooser->addOption("Checkerboard");
			typeChooser->addOption("Circles");
			typeChooser->setSelection(this->boardType);
			typeChooser->onSelectionChange += [this](const int & selectionIndex) {
				this->boardType = selectionIndex;
				this->updatePreviewMesh();
			};
			inspector->add(typeChooser);

			Utils::Gui::addIntSlider(this->sizeX, inspector)->onValueChange += sliderCallback;
			Utils::Gui::addIntSlider(this->sizeY, inspector)->onValueChange += sliderCallback;
			inspector->add(Widgets::Title::make("NB : Checkerboard size is\n counted by number of\n inner corners", Widgets::Title::Level::H3));
			inspector->add(Widgets::Spacer::make());

			auto spacingSlider = Widgets::Slider::make(this->spacing);
			spacingSlider->onValueChange += sliderCallback;
			inspector->add(spacingSlider);
		}

		//----------
		void Board::updatePreviewMesh() {
			this->previewMesh = ofxCv::makeBoardMesh(this->getBoardType(), this->getSize(), this->spacing, true);
		}
	}
}
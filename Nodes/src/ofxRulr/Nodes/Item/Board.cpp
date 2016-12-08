#include "pch_RulrNodes.h"
#include "Board.h"

#include "ofxCvGui.h"
#include "ofxCvMin.h"
#include "ofxRulr/Utils/Gui.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			Board::Board() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			void Board::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->updatePreviewMesh();

				auto view = make_shared<ofxCvGui::Panels::World>();
				view->onDrawWorld += [this](ofCamera &) {
					this->drawObject();
				};
				view->setGridEnabled(false);
#ifdef OFXCVGUI_USE_OFXGRABCAM
				view->getCamera().setCursorDrawEnabled(true);
				view->getCamera().setCursorDrawSize(this->parameters.spacing / 5.0f);
#endif
				this->view = view;

				auto & camera = view->getCamera();
				auto distance = this->parameters.spacing * MAX(this->parameters.sizeX, this->parameters.sizeY);
				camera.setPosition(0, 0, -distance);
				camera.lookAt(ofVec3f(), ofVec3f(0, -1, 0));
				camera.setNearClip(distance / 30.0f);
				camera.setFarClip(distance * 10.0f);
			}

			//----------
			string Board::getTypeName() const {
				return "Item::Board";
			}

			//----------
			ofxCvGui::PanelPtr Board::getPanel() {
				return this->view;
			}

			//----------
			void Board::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
			}

			//----------
			void Board::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);

				this->updatePreviewMesh();
			}

			//----------
			ofxCv::BoardType Board::getBoardType() const {
				switch (this->parameters.boardType) {
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
				return cv::Size(this->parameters.sizeX, this->parameters.sizeY);
			}

			//----------
			float Board::getSpacing() const {
				return this->parameters.spacing.get();
			}

			//----------
			vector<cv::Point3f> Board::getObjectPoints() const {
				if (this->parameters.offset.x == 0.0f && this->parameters.offset.y == 0.0f) {
					return ofxCv::makeBoardPoints(this->getBoardType(), this->getSize(), this->getSpacing(), this->parameters.offset.centered);
				}
				else {
					auto objectPoints = ofxCv::makeBoardPoints(this->getBoardType(), this->getSize(), this->getSpacing(), this->parameters.offset.centered);
					for (auto & objectPoint : objectPoints) {
						objectPoint += cv::Point3f(this->parameters.offset.x, this->parameters.offset.y, this->parameters.offset.z);
					}
					return objectPoints;
				}
			}

			//----------
			void Board::drawObject() const {
				this->previewMesh.draw();
			}

			//----------
			bool Board::findBoard(cv::Mat image, vector<cv::Point2f> & results, FindBoardMode findBoardMode) const {
				auto size = this->getSize();
				switch (findBoardMode) {
				case FindBoardMode::Raw:
					return ofxCv::findBoard(image, this->getBoardType(), size, results, false);
				case FindBoardMode::Optimized:
					return ofxCv::findBoard(image, this->getBoardType(), size, results, true);
				case FindBoardMode::Assistant:
					return ofxCv::findBoardWithAssistant(image, this->getBoardType(), size, results);
				default:
					return false;
				}
			}

			//----------
			void Board::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
				auto inspector = inspectArguments.inspector;
				
				auto sliderCallback = [this](ofParameter<float> &) {
					this->updatePreviewMesh();
				};

				auto typeChooser = make_shared<Widgets::MultipleChoice>("Board type");
				typeChooser->addOption("Checkerboard");
				typeChooser->addOption("Circles");
				typeChooser->setSelection(this->parameters.boardType);
				typeChooser->onValueChange += [this](const int & selectionIndex) {
					this->parameters.boardType = selectionIndex;
					this->updatePreviewMesh();
				};

				inspector->add(typeChooser);
				inspector->addParameterGroup(this->parameters.offset);

				Utils::Gui::addIntSlider(this->parameters.sizeX, inspector->getElementGroup())->onValueChange += sliderCallback;
				Utils::Gui::addIntSlider(this->parameters.sizeY, inspector->getElementGroup())->onValueChange += sliderCallback;
				inspector->add(new Widgets::Title("NB : Checkerboard size is\n counted by number of\n inner corners", Widgets::Title::Level::H3));
				
				inspector->add(new ofxCvGui::Widgets::LiveValue<string>("Warning", [this]() {
					if (this->parameters.boardType == 0) {
						//CHECKERBOARD

						bool xOdd = (int) this->parameters.sizeX & 1;
						bool yOdd = (int) this->parameters.sizeY & 1;
						if (xOdd && yOdd) {
							return "Size X and Size Y are both odd";
						}
						else if (!xOdd && !yOdd) {
							return "Size X and Size Y are both even";
						}
						else {
							return "";
						}
					}
					else {
						//CIRCLES

						return "";
					}
				}));
				inspector->add(new Widgets::Spacer());

				auto spacingSlider = new Widgets::Slider(this->parameters.spacing);
				spacingSlider->onValueChange += sliderCallback;
				inspector->add(spacingSlider);
			}

			//----------
			void Board::updatePreviewMesh() {
				this->previewMesh = ofxCv::makeBoardMesh(this->getBoardType(), this->getSize(), this->getSpacing(), true);
			}
		}
	}
}
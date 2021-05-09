#include "pch_RulrNodes.h"
#include "RandomPatternBoard.h"

#include "ofxRulr.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			RandomPatternBoard::RandomPatternBoard() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string RandomPatternBoard::getTypeName() const {
				return "Item::RandomPatternBoard";
			}

			//----------
			void RandomPatternBoard::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				this->rebuild();
				this->panel = ofxCvGui::Panels::makeImage(this->image);

			}

			//----------
			void RandomPatternBoard::update() {
				if (this->cachedBoardParameters.width != this->parameters.width
					|| this->cachedBoardParameters.height != this->parameters.height) {
					try {
						this->rebuild();
					}
					RULR_CATCH_ALL_TO_ERROR;
				}
			}

			//----------
			ofxCvGui::PanelPtr RandomPatternBoard::getPanel() {
				return this->panel;
			}

			//----------
			void RandomPatternBoard::populateInspector(ofxCvGui::InspectArguments& inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Rebuild", [this]() {
					try {
						this->rebuild();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
			}

			//----------
			void RandomPatternBoard::drawObject() const {

			}

			//----------
			bool RandomPatternBoard::findBoard(cv::Mat image, vector<cv::Point2f>& imagePoints, vector<cv::Point3f>& objectPoints, FindBoardMode findBoardMode, cv::Mat cameraMatrix, cv::Mat distortionCoefficients) const {
				if (!this->cornerFinder) {
					throw(ofxRulr::Exception("Corner finder not loaded"));
				}

				auto result = this->cornerFinder->computeObjectImagePointsForSingle(image);

				auto size = result[0].rows;
				if (size == 0) {
					return false;
				}

				imagePoints.resize(size);
				memcpy(imagePoints.data(), result[0].data, size * sizeof(cv::Point2f));

				objectPoints.resize(size);
				memcpy(objectPoints.data(), result[1].data, size * sizeof(cv::Point3f));

				return true;
			}

			//----------
			void RandomPatternBoard::rebuild() {
				cv::Mat image;
				
				this->patternGenerator.reset();
				this->cornerFinder.reset();

				// Create the pattern
				{
					this->patternGenerator = make_shared<cv::randpattern::RandomPatternGenerator>(this->parameters.width, this->parameters.height);

					this->patternGenerator->generatePattern();
					image = this->patternGenerator->getPattern();
					this->image.allocate(image.cols, image.rows, ofImageType::OF_IMAGE_GRAYSCALE);
					image.copyTo(ofxCv::toCv(this->image.getPixels()));
					this->image.update();
				}

				// Create the finder
				{
					this->cornerFinder = make_shared<cv::randpattern::RandomPatternCornerFinder>(this->parameters.width
						, this->parameters.height);
					this->cornerFinder->loadPattern(image);
				}

				this->cachedBoardParameters = this->parameters;
			}
		}
	}
}
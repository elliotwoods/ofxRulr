#include "pch_Plugin_Reworld.h"
#include "CameraTest.h"
#include "Router.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Reworld {
			//---------
			CameraTest::CameraTest()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//---------
			string
				CameraTest::getTypeName() const
			{
				return "Reworld::CameraTest";
			}

			//---------
			void
				CameraTest::init()
			{
				this->addInput<Router>();
				this->addInput<Item::Camera>();

				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->manageParameters(this->parameters);

				auto verticalStrip = ofxCvGui::Panels::Groups::makeStrip(ofxCvGui::Panels::Groups::Strip::Vertical);
				{
					auto horizontalStrip = ofxCvGui::Panels::Groups::makeStrip();
					{
						auto panel = ofxCvGui::Panels::makeImage(this->preview);

						panel->onDrawImage += [this](ofxCvGui::DrawImageArguments& args) {
							ofPushMatrix();
							{
								ofMultMatrix(this->homography);

								// Draw the image
								ofPushStyle();
								{
									ofSetColor(255, this->parameters.mask.opacity.get() * 255.0f);

									switch (this->parameters.mask.blendMode.get().get()) {
									case BlendMode::Alpha:
										ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ALPHA);
										break;
									case BlendMode::Add:
										ofEnableBlendMode(ofBlendMode::OF_BLENDMODE_ADD);
										break;
									default:
										break;
									}
									ofxAssets::image("ofxRulr::Reworld::Shroud").draw(0, 0);
								}
								ofPopStyle();

								// Draw mask circle
								ofPushStyle();
								{
									ofNoFill();
									ofSetCircleResolution(50);
									ofDrawCircle(256, 256, this->parameters.mask.radius);
								}
								ofPopStyle();
							}
							ofPopMatrix();

							ofPushStyle();
							{
								for (int i = 0; i < 4; i++) {
									auto selected = this->isBeingInspected() && i == this->selectedCornerIndex;
									const auto& targetCorner = this->targetCorners[i];
									if (selected && !this->parameters.mask.locked) {
										ofSetColor(ofxCvGui::Utils::getBeatingSelectionColor());
									}
									else {
										ofSetColor(255, 255, 255);
									}
									ofPushMatrix();
									{
										ofTranslate(targetCorner);
										ofDrawLine(-10, 0, 10, 0);
										ofDrawLine(0, -10, 0, 10);
									}
									ofPopMatrix();
								}
							}
							ofPopStyle();
						};

						auto panelWeak = weak_ptr<ofxCvGui::Panels::Image>(panel);

						panel->onKeyboard += [this](ofxCvGui::KeyboardArguments& args) {
							if (this->parameters.mask.locked) {
								return;
							}

							glm::vec2 movement{ 0.0f, 0.0f };

							if (args.action == ofxCvGui::KeyboardArguments::Action::Pressed
								&& this->isBeingInspected()) {
								switch (args.key) {
								case OF_KEY_LEFT:
									movement.x -= 1;
									break;
								case OF_KEY_RIGHT:
									movement.x += 1;
									break;
								case OF_KEY_UP:
									movement.y -= 1;
									break;
								case OF_KEY_DOWN:
									movement.y += 1;
									break;
								case OF_KEY_TAB:
									if (ofGetKeyPressed(OF_KEY_SHIFT)) {
										this->selectedCornerIndex--;
										if (this->selectedCornerIndex < 0) {
											this->selectedCornerIndex = 3;
										}
									}
									else {
										this->selectedCornerIndex++;
										if (this->selectedCornerIndex > 3) {
											this->selectedCornerIndex = 0;
										}
									}
								deafult:
									break;
								}
								this->transformDirty = true;
							}

							// Fine movements
							if (ofGetKeyPressed(OF_KEY_SHIFT)) {
								movement *= 0.2f;
							}

							// Move one or move all
							if (ofGetKeyPressed(OF_KEY_ALT)) {
								for (auto& targetCorner : this->targetCorners) {
									targetCorner += movement;
								}
							}
							else {
								auto& targetCorner = this->targetCorners[this->selectedCornerIndex];
								targetCorner += movement;
							}
						};

						panel->onMouse += [this, panelWeak](ofxCvGui::MouseArguments& args) {
							if (!this->isBeingInspected()) {
								return;
							}

							if (this->parameters.mask.locked) {
								return;
							}

							auto panel = panelWeak.lock();
							args.takeMousePress(panel);

							if (args.isDragging(panel)) {
								auto beforeMovementPanel = args.local - args.movement;
								auto afterMovementPanel = args.local;

								auto imageToPanel = glm::inverse(panel->getPanelToImageTransform());

								auto beforeMovementImage = ofxCeres::VectorMath::applyTransform(imageToPanel, glm::vec3(beforeMovementPanel, 1.0f));
								auto afterMovementImage = ofxCeres::VectorMath::applyTransform(imageToPanel, glm::vec3(afterMovementPanel, 1.0f));

								auto movementImage = afterMovementImage - beforeMovementImage;

								// Fine movements
								if (ofGetKeyPressed(OF_KEY_SHIFT)) {
									movementImage *= 0.2f;
								}

								// Move one or move all
								if (ofGetKeyPressed(OF_KEY_ALT)) {
									for (auto& targetCorner : this->targetCorners) {
										targetCorner.x += movementImage.x;
										targetCorner.y += movementImage.y;
									}
								}
								else {
									auto& targetCorner = this->targetCorners[this->selectedCornerIndex];
									targetCorner.x += movementImage.x;
									targetCorner.y += movementImage.y;
								}

								this->transformDirty = true;
							}
						};

						horizontalStrip->add(panel);
					}
					{
						auto panel = ofxCvGui::Panels::makeImage(this->captureRegion.preview);
						horizontalStrip->add(panel);
					}
					verticalStrip->add(horizontalStrip);
				}
				{
					auto panel = ofxCvGui::Panels::makeImage(this->scanArea.preview);
					{
						panel->onDrawImage += [this](ofxCvGui::DrawImageArguments& args) {
							// Draw live data
							ofPushStyle();
							{
								ofSetColor(100, 100, 200);

								ofDrawCircle(this->positionToScanArea(this->currentState.position), 10);

								ofNoFill();
								ofDrawCircle(this->positionToScanArea(this->currentState.targetPosition), 15);
							}
							ofPopStyle();

							ofPushStyle();
							{
								ofNoFill();

								ofSetColor(100);

								// Draw lines between scan positions
								this->scanArea.linePreview.draw();

								// Draw first scan position
								if (!this->scanArea.iterationPositionsScanArea.empty()) {
									const auto& position = this->scanArea.iterationPositionsScanArea.front();
									ofDrawCircle(position, 10);
								}

								// Draw all scan positions
								ofSetColor(255);
								ofSetCircleResolution(4);
								for (const auto& position : this->scanArea.iterationPositionsScanArea) {
									ofDrawCircle(position, 3);
								}

							}
							ofPopStyle();
						};
					}
					verticalStrip->add(panel);
				}
				this->panel = verticalStrip;

				this->sourceCorners = {
					{ 125, 25 }
					, { 386, 25 }
					, { 386, 486 }
					, { 125, 486 }
				};

				// initialise target as same as source for test
				this->targetCorners = this->sourceCorners;
			}

			//---------
			void
				CameraTest::update()
			{
				auto camera = this->getInput<Item::Camera>();
				if (camera) {
					auto grabber = camera->getGrabber();
					if (grabber) {
						this->isFrameNew = grabber->isFrameNew();
						if (this->isFrameNew) {
							// copy the preview instead of undistorting again here
							this->preview = camera->getUndistortedPreview();
							this->image = ofxCv::toCv(this->preview.getPixels());
						}
					}
					else {
						this->isFrameNew = false;
					}
				}
				else {
					this->isFrameNew = false;
				}

				if (this->transformDirty) {
					auto homography = ofxCv::findHomography(ofxCv::toCv(this->sourceCorners), ofxCv::toCv(this->targetCorners));
					this->homography[0][0] = homography.at<double>(0, 0);
					this->homography[1][0] = homography.at<double>(1, 0);
					this->homography[2][0] = 0.0f;
					this->homography[3][0] = homography.at<double>(2, 0);

					this->homography[0][1] = homography.at<double>(0, 1);
					this->homography[1][1] = homography.at<double>(1, 1);
					this->homography[2][1] = 0.0f;
					this->homography[3][1] = homography.at<double>(2, 1);

					this->homography[2][0] = 0.0f;
					this->homography[2][1] = 0.0f;
					this->homography[2][2] = 1.0f;
					this->homography[2][3] = 0.0f;

					this->homography[0][3] = homography.at<double>(0, 2);
					this->homography[1][3] = homography.at<double>(1, 2);
					this->homography[2][3] = 0.0f;
					this->homography[3][3] = 1.0f;

					// the cols/rows are the other way around
					this->homography = glm::transpose(this->homography);

					this->transformDirty = false;
				}

				// Perform capture update
				{
					if (this->isFrameNew && !this->image.empty()) {
						auto squareImageCorners = this->sourceCorners;

						// scale by radius
						for (auto& corner : squareImageCorners) {
							corner = (corner - glm::vec2(256, 256)) * 256 / this->parameters.mask.radius.get() + glm::vec2(256, 256);
						}

						auto homography = ofxCv::findHomography(ofxCv::toCv(this->targetCorners)
							, ofxCv::toCv(squareImageCorners));

						cv::warpPerspective(this->image
							, this->captureRegion.squareImage
							, homography
							, cv::Size(512, 512));
						ofxCv::copy(this->captureRegion.squareImage, this->captureRegion.preview.getPixels());
						this->captureRegion.preview.update();
					}
				}

				// Scan area
				{
					const auto& res = this->parameters.scanArea.resolution.get();

					// Allocate
					{
						if (this->scanArea.image.cols != res
							|| this->scanArea.image.rows != res) {
							this->scanArea.image = cv::Mat(res, res, CV_8UC3);
							this->scanArea.previewDirty = true;
						}
					}

					// Update preview
					{
						if (this->scanArea.previewDirty) {
							ofxCv::copy(this->scanArea.image, this->scanArea.preview.getPixels());
							this->scanArea.preview.update();
							this->scanArea.previewDirty = false;
						}
					}
				}

				// perform scan routine
				this->updateScanRoutine();
			}

			//---------
			void
				CameraTest::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addLiveValue<string>("Alt", []() { return "Move all at once"; });
				inspector->addLiveValue<string>("Shift", []() { return "Move at 0.2x speed"; });
				inspector->addLiveValue<string>("[UP]/[DOWN]/[LEFT]/[RIGHT]", []() { return "Move by 1px"; });
				inspector->addButton("Calculate capture iterations", [this]() {
					try {
						this->calculateIterations();
					}
					RULR_CATCH_ALL_TO_ALERT;
					});
				inspector->addButton("Sort iterations by distance", [this]() {
					try {
						this->sortIterationsByDistance();
					}
					RULR_CATCH_ALL_TO_ALERT;
					})->addToolTip("Find shortest route");

					inspector->addButton("Scan", [this]() {
						this->startScanRoutine();
						}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//---------
			void
				CameraTest::serialize(nlohmann::json& json) const
			{
				Utils::serialize(json["targetCorners"], this->targetCorners);
			}

			//---------
			void
				CameraTest::deserialize(const nlohmann::json& json)
			{
				if (json.contains("targetCorners")) {
					Utils::deserialize(json["targetCorners"], this->targetCorners);
				}
			}

			//---------
			ofxCvGui::PanelPtr
				CameraTest::getPanel()
			{
				return this->panel;
			}

			//---------
			glm::vec2
				CameraTest::positionToScanArea(const glm::vec2& position)
			{
				const auto scanAreaRadius = (float)this->parameters.scanArea.resolution.get() / 2.0f;
				return {
					scanAreaRadius * (1 + position.x)
					, scanAreaRadius * (1 + position.y)
				};
			}

			//---------
			glm::vec2
				CameraTest::scanAreaToPosition(const glm::vec2& scanArea)
			{
				const auto scanAreaRadius = (float)this->parameters.scanArea.resolution.get() / 2.0f;
				return {
					(scanArea.x - scanAreaRadius) / scanAreaRadius
					, (scanArea.y - scanAreaRadius) / scanAreaRadius
				};
			}

			//---------
			glm::vec2
				CameraTest::positionToPolar(const glm::vec2& position)
			{
				return {
					glm::length(position)
					, atan2(position.y, position.x)
				};
			}

			//---------
			glm::vec2
				CameraTest::polarToPosition(const glm::vec2& polar)
			{
				const auto& r = polar[0];
				const auto& theta = polar[1];

				return {
					r * cos(theta)
					, r * sin(theta)
				};
			}

			//---------
			glm::vec2
				CameraTest::polarToAxes(const glm::vec2& polar)
			{
				const auto& r = polar[0];
				const auto& theta = polar[1];

				// axes norm coordinates are offset by half rotation from polar
				// (for axes, left = 0; for polar, right = 0)
				const auto thetaNorm = theta / TWO_PI - 0.5f;

				// our special sauce for our lenses
				return {
					thetaNorm - (1 - r) * 0.25 + 0.5
					, thetaNorm + (1 - r) * 0.25 + 0.5
				};
			}

			//---------
			glm::vec2
				CameraTest::axesToPolar(const glm::vec2& axes)
			{
				auto a = axes[0];
				auto b = axes[1];

				// ignore cycles
				auto flattenCycle = [](float x) {
					// bring it into -1...1
					x = fmodf(x, 1);
					if (x < 0) {
						x += 1;
					}
					return x;
				};

				a = flattenCycle(a);
				b = flattenCycle(b);

				auto r = 2 * a - 2 * b + 1;
				auto thetaNorm = (a + b - 1) / 2;

				// Somehow this seems to work
				if (r > 1.0f) {
					r = 1 - (r - 1);
				}
				if (r < 0.0f) {
					thetaNorm += 0.5f;
					r = -r;
				}

				auto theta = (thetaNorm + 0.5f) * TWO_PI;

				return {
					r
					, theta
				};
			}

			//---------
			Router::Address
				CameraTest::getTargetAddress() const
			{
				return Router::Address{
					this->parameters.target.column.get()
					, (uint8_t)this->parameters.target.portal.get()
				};
			}

			//---------
			void
				CameraTest::calculateIterations()
			{
				switch (this->parameters.capture.iterations.mode.get().get()) {
				case IterationMode::Polar:
					this->calculateIterationsPolar();
					break;
				case IterationMode::Cartesian:
					this->calculateIterationsCartesian();
					break;
				default:
					break;
				}
			}


			//---------
			void
				CameraTest::calculateIterationsPolar()
			{
				vector<glm::vec2> prismPositions;

				const auto minR = this->parameters.capture.iterations.polar.minR.get();
				const auto maxR = this->parameters.capture.iterations.polar.maxR.get();
				const auto rSteps = this->parameters.capture.iterations.polar.rSteps.get();
				const auto minTheta = this->parameters.capture.iterations.polar.minTheta.get();
				const auto maxTheta = this->parameters.capture.iterations.polar.maxTheta.get();
				const auto thetaSteps = this->parameters.capture.iterations.polar.thetaSteps.get();
				const auto scaleThetaStepsWithR = this->parameters.capture.iterations.polar.scaleThetaStepsWithR.get();

				// Step up through r
				auto rStep = (maxR - minR) / rSteps;
				for (float r = minR; r <= maxR; r += rStep) {
					// Calculate theta step (e.g. if normalised)
					float thetaStepsHere = scaleThetaStepsWithR
						? ofMap(r, minR, maxR, 1, thetaSteps)
						: thetaSteps;

					// Step up through theta
					auto thetaStep = (maxTheta - minTheta) / thetaStepsHere;
					for (float theta = minTheta; theta <= maxTheta; theta += thetaStep) {
						// we offset theta by pi because the overflow line is on the left size of the prism system
						auto theta2 = theta + PI;

						glm::vec2 prismPosition{
							r * cos(theta2)
							, r * sin(theta2)
						};

						prismPositions.push_back(prismPosition);
					}
				}

				this->scanArea.iterationPositionsPrism = prismPositions;
				this->calculateScanAreaPositions();
			}

			//---------
			void
				CameraTest::calculateIterationsCartesian()
			{
				vector<glm::vec2> prismPositions;

				const auto steps = this->parameters.capture.iterations.cartesian.steps.get();
				const auto step = 2.0f / (float)steps;

				for (int iy = 0; iy <= steps; iy++) {
					float y = (float)iy * 2.0f / steps - 1.0f;
					for (int ix = 0; ix <= steps; ix++) {
						float x = (float)ix * 2.0f / steps - 1.0f;

						auto r = glm::length(glm::vec2{ x, y });

						// skip outside of circle
						if (r > this->parameters.capture.iterations.cartesian.maxR) {
							continue;
						}

						prismPositions.push_back({
							x
							, y
							});
					}
				}

				this->scanArea.iterationPositionsPrism = prismPositions;
				this->calculateScanAreaPositions();
			}

			//---------
			void
				CameraTest::calculateScanAreaPositions()
			{
				vector<glm::vec2> scanAreaPositions;

				for (const auto& prismPosition : this->scanArea.iterationPositionsPrism) {
					scanAreaPositions.push_back(this->positionToScanArea(prismPosition));
				}
				this->scanArea.iterationPositionsScanArea = scanAreaPositions;

				// update preview
				{
					this->scanArea.linePreview.clear();
					for (const auto& point : this->scanArea.iterationPositionsScanArea) {
						this->scanArea.linePreview.addVertex({
							point.x
							, point.y
							, 0.0f
							});
					}
				}
			}

			//---------
			void CameraTest::sortIterationsByDistance()
			{
				// ignore small problems
				if (this->scanArea.iterationPositionsPrism.size() < 2) {
					return;
				}

				// calculate axis values for all positions
				vector<glm::vec2> axisValues;
				for (const auto& position : this->scanArea.iterationPositionsPrism) {
					auto polar = CameraTest::positionToPolar(position);
					auto axes = CameraTest::polarToAxes(polar);
					axisValues.push_back(axes);
				}

				// work with indices
				set<int> inputIndicesRemaining;
				for (int i = 0; i < axisValues.size(); i++) {
					inputIndicesRemaining.insert(i);
				}
				vector<int> inputIndicesSorted;

				// add first element as first input element
				glm::vec2* axisValue = &axisValues[0];
				inputIndicesSorted.push_back(0);
				inputIndicesRemaining.erase(0);

				// perform the sort
				while (!inputIndicesRemaining.empty()) {
					// find the closest to the current axisValue
					float minDistance = std::numeric_limits<float>::max();
					int minIndex = -1;
					for (const auto& i : inputIndicesRemaining) {
						const auto& otherAxisValue = axisValues[i];
						auto distance = glm::distance(*axisValue, otherAxisValue);
						if (distance < minDistance) {
							minDistance = distance;
							minIndex = i;
						}
					}

					// set the closest as next point
					inputIndicesSorted.push_back(minIndex);
					inputIndicesRemaining.erase(minIndex);
					axisValue = &axisValues[minIndex];
				}

				vector<glm::vec2> prismPositions;
				for (const auto i : inputIndicesSorted) {
					prismPositions.push_back(this->scanArea.iterationPositionsPrism[i]);
				}
				this->scanArea.iterationPositionsPrism = prismPositions;
				this->calculateScanAreaPositions();
			}

			//---------
			void CameraTest::startScanRoutine()
			{
				auto router = this->getInput<Router>();
				try {
					if (!router) {
						throw(ofxRulr::Exception("Cannot start without router"));
					}

					// check we cannot to router
					router->test();

					// move to first position
					this->scanRoutine.currentIndex = 0;
					router->setPosition(this->getTargetAddress(), this->scanArea.iterationPositionsPrism[0]);

					// set state as running
					this->parameters.state.set(State::Running);
				}
				RULR_CATCH_ALL_TO({
					this->parameters.state.set(State::Fail);
					throw(e);
					})
			}


			//---------
			void CameraTest::updateScanRoutine()
			{
				auto router = this->getInput<Router>();
				if (!router) {
					this->parameters.state.set(State::Fail);
				}

				// Check if finished
				if (this->scanRoutine.currentIndex >= this->scanArea.iterationPositionsPrism.size()) {
					this->parameters.state.set(State::Finish);
					return;
				}

				const auto targetAddress = this->getTargetAddress();

				try {
					try {
						// Poll
						bool isPollFrame = false;
						if (this->parameters.state.get() == State::Running
							|| this->parameters.state.get() == State::Poll) {
							if (chrono::system_clock::now() > this->scanRoutine.lastPoll + std::chrono::milliseconds((int)(this->parameters.capture.movements.pollFrequency.get() * 1000.0f))) {
								router->poll(targetAddress);

								// Pull position
								this->currentState.position = router->getPosition(targetAddress);
								this->currentState.targetPosition = router->getTargetPosition(targetAddress);

								isPollFrame = true;
							}
						}

						// Further functions are only for run mode
						if (this->parameters.state.get() != State::Running) {
							return;
						}

						if (isPollFrame) {
							// Push the position (we do this for redundancy to avoid missed messages on RS485 side)
							router->setPosition(targetAddress, this->scanArea.iterationPositionsPrism[this->scanRoutine.currentIndex]);
						}

						// Check if it's reached a target position
						if (router->isInPosition(targetAddress)) {
							// capture!
							{
								const auto& currentTargetPositon = this->scanArea.iterationPositionsScanArea[this->scanRoutine.currentIndex];
								const auto captureResolution = this->captureRegion.squareImage.rows;
								if (captureResolution > 0) {
									auto targetRect = cv::Rect2i(currentTargetPositon.x - captureResolution / 2
										, currentTargetPositon.y - captureResolution / 2
										, captureResolution
										, captureResolution);

									const auto scanAreaRect = ofRectangle(0, 0, this->scanArea.image.cols, this->scanArea.image.rows);

									// check if targetRect is inside scan area rectangle
									if (scanAreaRect.inside(ofxCv::toOf(targetRect))) {
										this->scanArea.image(targetRect) = this->captureRegion.squareImage;
										this->scanArea.previewDirty = true;
									}
								}
							}
							// move to next
							this->scanRoutine.currentIndex++;

							// check if any positions remaining
							if (this->scanRoutine.currentIndex >= this->scanArea.iterationPositionsPrism.size()) {
								this->parameters.state.set(State::Finish);
								return;
							}

							// signal to move to next prism position
							router->setPosition(targetAddress, this->scanArea.iterationPositionsPrism[this->scanRoutine.currentIndex]);
						}
					}
					RULR_CATCH_ALL_TO({
						this->parameters.state.set(State::Fail);
						throw(e);
						})
				}
				RULR_CATCH_ALL_TO_ERROR;
			}
		}
	}
}
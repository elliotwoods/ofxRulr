#include "pch_Plugin_Scrap.h"
#include "DrawMoon.h"

#include "Lasers.h"
#include "Moon.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//------------
			DrawMoon::DrawMoon()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//------------
			string
				DrawMoon::getTypeName() const
			{
				return "AnotherMoon::DrawMoon";
			}

			//------------
			void
				DrawMoon::init()
			{
				RULR_NODE_DRAW_WORLD_LISTENER;;
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;

				this->manageParameters(this->parameters);

				auto moonInput = this->addInput<Moon>();
				moonInput->onNewConnection += [this](shared_ptr<Moon> moonNode) {
					moonNode->onMoonChange.addListener([this]() {
						this->moonIsNewNotify = true;
						}, this);
				};
				moonInput->onDeleteConnection += [this](shared_ptr<Moon> moonNode) {
					moonNode->onMoonChange.removeListeners(this);
				};

				this->addInput<Lasers>();
			}

			//------------
			void
				DrawMoon::update()
			{
				if (ofxRulr::isActive(this, this->parameters.live.get())) {
					try {
						this->drawLasers(this->parameters.errorIfOutsideRange);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}


				{
					this->moonIsNew = this->moonIsNewNotify;
					this->moonIsNewNotify = false;
				}

				if (this->moonIsNew && ofxRulr::isActive(this, this->parameters.onMoonChange.get())) {
					try {
						this->drawLasers(this->parameters.errorIfOutsideRange);
					}
					RULR_CATCH_ALL_TO_ERROR;
				}

				{
					auto now = chrono::system_clock::now();
					auto timeSinceLastUpdate = this->schedule.lastUpdate - now;
					auto interval = std::chrono::milliseconds((int) (this->parameters.schedule.interval.get() * 1000));
					if (timeSinceLastUpdate >= interval) {
						try {
							this->drawLasers(this->parameters.errorIfOutsideRange);
						}
						RULR_CATCH_ALL_TO_ERROR;
					}
				}
			}

			//------------
			void
				DrawMoon::drawWorldStage()
			{
				if (this->parameters.debugDraw.enabled) {
					if (ofxRulr::isActive(this, this->parameters.debugDraw.lines.get())) {
						for (auto& preview : this->previews) {
							ofPushStyle();
							{
								ofSetColor(preview.color);
								preview.polyline.draw();
							}
							ofPopStyle();
						}
					}
				}
			}

			//------------
			void
				DrawMoon::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Draw lasers", [this]() {
					try {
						this->drawLasers(true);
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);

				inspector->addButton("Search height", [this]() {
					try {
						auto text = ofSystemTextBoxDialog("Search height range");
						if (text.empty()) {
							return;
						}
						auto range = ofToFloat(text);
						this->searchHeight(range);
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, 's');
			}

			//------------
			void
				DrawMoon::drawLasers(bool throwIfPictureOutsideLimits)
			{
				this->throwIfMissingAnyConnection();

				this->previews.clear();

				auto lasersNode = this->getInput<Lasers>();
				auto lasers = lasersNode->getLasersSelected();

				auto moonNode = this->getInput<Moon>();
				auto moonPosition = moonNode->getPosition();
				auto moonRadius = moonNode->getRadius();

				const auto& resolution = this->parameters.resolution.get();

				if (this->parameters.debugDraw.enabled) {
					// pre-allocate the previews so we don't need to move them around
					this->previews.resize(lasers.size());
				}

				auto debugOutput = this->previews.begin();
				for (auto laser : lasers) {
					auto laserPosition = laser->getRigidBody()->getPosition();
					auto circleNormal = glm::normalize(moonPosition - laserPosition);

					// Calcualte points for circle
					vector<glm::vec3> points;
					{
						glm::vec3 u, v;
						if (circleNormal == glm::vec3(0, 0, 1)
							|| circleNormal == glm::vec3(0, 0, -1)) {
							// special case
							u = glm::vec3(1, 0, 0);
							v = glm::vec3(0, 1, 0);
						}
						else {
							u = glm::normalize(glm::cross(circleNormal, glm::vec3(0, 0, 1)));
							v = glm::cross(circleNormal, u);
						}

						for (int i = 0; i < resolution; i++) {
							auto theta = (float)i / (float)resolution * TWO_PI;
							points.push_back(moonPosition
								+ u * cos(theta) * moonRadius
								+ v * sin(theta) * moonRadius);
						}
					}

					// Update 3D preview
					if (this->parameters.debugDraw.enabled) {
						debugOutput->polyline.addVertices(points);
						debugOutput->polyline.close();
						debugOutput->color = laser->color;
						debugOutput++;
					}

					// Project image points for 3D points
					laser->drawWorldPoints(points);
					
					// Check if image is outisde of limits
					if (throwIfPictureOutsideLimits) {
						const auto& picture = laser->getLastPicture();
						for (auto point : picture) {
							if (point.x < -1 || point.x > 1
								|| point.y < -1 || point.y > 1) {
								throw(ofxRulr::Exception("Picture is outside range for Laser " + laser->getLabelName() + " (" + ofToString(moonPosition) + ")"));
							}
						}
					}

					this->schedule.lastUpdate = chrono::system_clock::now();
				}
			}

			//------------
			void
				DrawMoon::searchHeight(float range)
			{
				this->throwIfMissingAConnection<Moon>();
				auto moon = this->getInput<Moon>();

				auto midPosition = moon->getPosition();
				auto midHeight = midPosition.y;

				auto minSearchHeight = midHeight - range;
				auto maxSearchHeight = midHeight + range;
				auto minHeight = maxSearchHeight;
				auto maxHeight = minSearchHeight;
				bool anyHeightOK = false;

				const float step = 0.1;
				const int steps = (maxSearchHeight - minSearchHeight) / step + 1;
				Utils::ScopedProcess scopedProcess("Searching heights", true, steps);

				for (float height = minSearchHeight; height <= maxSearchHeight; height += step) {
					try {
						Utils::ScopedProcess scopedProcessHeight(ofToString(height), steps);
						auto position = midPosition;
						position.y = height;
						moon->setPosition(position);
						this->drawLasers(true);

						// success
						anyHeightOK = true;
						if (height < minHeight) {
							minHeight = height;
						}
						if (height > maxHeight) {
							maxHeight = height;
						}

						scopedProcessHeight.end();
					}
					catch (...) {

					}
				}

				if (anyHeightOK) {
					cout << "Minimum height : " << minHeight << endl;
					cout << "Maximum height : " << maxHeight << endl;
				}
				else {
					throw(ofxRulr::Exception("All heights failed"));
				}

				scopedProcess.end();
			}
		}
	}
}

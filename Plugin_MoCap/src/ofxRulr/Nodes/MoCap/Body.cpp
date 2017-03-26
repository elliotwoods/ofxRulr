#include "pch_Plugin_MoCap.h"
#include "Body.h"

namespace ofxRulr {
	namespace Nodes {
		namespace MoCap {
#pragma mark Marker
			//----------
			Body::Marker::Marker() {
				RULR_NODE_SERIALIZATION_LISTENERS;
			}

			//----------
			std::string Body::Marker::getDisplayString() const {
				stringstream ss;
				ss << "X : " << this->position.get().x << endl;
				ss << "Y : " << this->position.get().y << endl;
				ss << "Z : " << this->position.get().z;
				return ss.str();
			}

			//----------
			void Body::Marker::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->position);
			}

			//----------
			void Body::Marker::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->position);
			}

#pragma mark Body
			//----------
			Body::Body() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("MoCap::icon"));
			}

			//----------
			std::string Body::getTypeName() const {
				return "MoCap::Body";
			}

			//----------
			void Body::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->onTransformChange += [this]() {
					this->invalidateBodyDescription();
				};

				auto panel = ofxCvGui::Panels::makeWorld();
				panel->setGridEnabled(false);
				panel->onDrawWorld += [this](ofCamera & camera) {
					auto bodyDescription = this->getBodyDescription();
					if (!bodyDescription) {
						return;
					}

					float maxDistanceSquared = 0.0f;

					//draw marker spheres
					{
						ofPushStyle();
						{
							for (int i = 0; i < bodyDescription->markerCount; i++) {
								ofSetColor(bodyDescription->markers.colors[i]);
								ofDrawSphere(bodyDescription->markers.positions[i], bodyDescription->markerDiameter / 2.0f);
								maxDistanceSquared = max(maxDistanceSquared, bodyDescription->markers.positions[i].lengthSquared());
							}
						}
						ofPopStyle();
					}

					//draw grid
					{
						float gridSize = 1.0f;
						const int steps = 5;
						if (bodyDescription->markerCount > 0) {
							gridSize = pow(floor(log10(sqrt(maxDistanceSquared) * 2)), 10) / 2;
						}

						ofPushMatrix();
						{
							ofDrawAxis(0.1f);
							ofRotate(90, 0, 0, +1);

							ofPushStyle();
							{
								ofSetColor(100);
								ofDrawGridPlane(gridSize / 5, 5, true);
							}
							ofPopStyle();
						}
						ofPopMatrix();
					}
				};

				this->panel = panel;
			}

			//----------
			void Body::update() {
				//apply any calculated matrices
				{
					bool receivedTransform = false;
					ofMatrix4x4 transform;
					while (this->transformIncoming.tryReceive(transform)) {
						receivedTransform = true;
					}
					if (receivedTransform) {
						this->setTransform(transform);
					}
				}

				//rebuild body description
				if (!this->getBodyDescription()) {
					auto bodyDescription = make_shared<Description>();
					
					auto markers = this->markers.getSelection();
					for (size_t i = 0; i < markers.size(); i++) {
						bodyDescription->markers.positions.push_back(markers[i]->position);
						bodyDescription->markers.colors.push_back(markers[i]->color);
						bodyDescription->markers.IDs.push_back(i);
					}

					bodyDescription->markerDiameter = this->parameters.markerDiameter;
					bodyDescription->markerCount = markers.size();
					
					bodyDescription->modelTransform = this->getTransform();
					ofxCv::decomposeMatrix(bodyDescription->modelTransform
						, bodyDescription->rotationVector
						, bodyDescription->translation);

					{
						auto lock = unique_lock<mutex>(this->bodyDescriptionMutex);
						this->bodyDescription = bodyDescription;
					}
				}

				//update world history
				{
					auto selection = this->markers.getSelection();
					for (auto marker : selection) {
						marker->worldHistory.push_back(marker->position * this->getTransform());
						auto maxSize = ofGetFrameRate() * this->parameters.drawStyle.historyTrailLength;
						while (marker->worldHistory.size() > maxSize) {
							marker->worldHistory.pop_front();
						}
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr Body::getPanel() {
				return this->panel;
			}

			//----------
			void Body::drawObject() const {
				auto markers = this->markers.getSelection();

				if (this->parameters.drawStyle.useColorsInWorld) {
					ofPushStyle();
					{
						for (const auto & marker : markers) {
							ofSetColor(marker->color);
							ofDrawSphere(marker->position.get(), this->parameters.markerDiameter / 2.0f);
						}
					}
					ofPopStyle();
				}
				else {
					for (const auto & marker : markers) {
						ofDrawSphere(marker->position.get(), this->parameters.markerDiameter / 2.0f);
					}
				}
			}

			//----------
			void Body::drawWorld() const {
				auto selection = this->markers.getSelection();
				ofPushStyle();
				{
					ofEnableAlphaBlending();
					for (auto marker : selection) {
						ofMesh trail;
						trail.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
						ofColor color = this->parameters.drawStyle.useColorsInWorld.get() ? marker->color : ofColor(255);

						int pointIndex = 0;
						auto historyLength = marker->worldHistory.size();
						for (auto point : marker->worldHistory) {
							trail.addVertex(point);
							color.a = ofMap(pointIndex++
								, 0, (float) historyLength
								, 0.0f, 255.0f
								, false);
							trail.addColor(color);
						}
						trail.draw();
					}
				}
				ofPopStyle();
				
			}

			//----------
			void Body::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addTitle("Markers :", ofxCvGui::Widgets::Title::H2);
				inspector->addParameterGroup(this->parameters);
				this->markers.populateWidgets(inspector);
				inspector->addButton("Add marker", [this]() {
					auto inputText = ofSystemTextBoxDialog("Marker position [m] 'x, y, z'");
					if (!inputText.empty()) {
						stringstream inputTextStream(inputText);
						ofVec3f position;
						inputTextStream >> position;

						auto marker = make_shared<Marker>();
						marker->position = position;
						this->markers.add(marker);

						this->invalidateBodyDescription();
					}
				});
				inspector->addButton("Load CSV...", [this]() {
					try {
						this->loadCSV();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
				inspector->addButton("Save CSV...", [this]() {
					try {
						this->saveCSV();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			void Body::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->parameters);
				this->markers.serialize(json["markers"]);
			}

			//----------
			void Body::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->parameters);
				this->markers.deserialize(json["markers"]);
				this->invalidateBodyDescription();
			}

			//----------
			shared_ptr<Body::Description> Body::getBodyDescription() const {
				{
					auto lock = unique_lock<mutex>(this->bodyDescriptionMutex);
					return this->bodyDescription;
				}
			}

			//----------
			float Body::getMarkerDiameter() const {
				return this->parameters.markerDiameter.get();
			}

			//----------
			void Body::loadCSV(string filename /*= ""*/) {
				if (filename == "") {
					auto result = ofSystemLoadDialog("Select CSV");
					if (!result.bSuccess) {
						return;
					}
					filename = result.filePath;
				}

				//load file into string
				ifstream file(filename, ios::in);
				string fileString = string(istreambuf_iterator<char>(file)
					, istreambuf_iterator<char>());
				file.close();

				this->markers.clear();

				//split into lines
				auto lines = ofSplitString(fileString, "\n", true, true);
				for (auto line : lines) {
					auto stringCoords = ofSplitString(line, ", ", true, true);
					if (stringCoords.size() < 3) {
						continue;
					}

					ofVec3f position(ofToFloat(stringCoords[0])
						, ofToFloat(stringCoords[1])
						, ofToFloat(stringCoords[2]));

					auto marker = make_shared<Marker>();
					marker->position = position;
					this->markers.add(marker);
				}

				this->invalidateBodyDescription();
			}

			//----------
			void Body::saveCSV(string filename /*= ""*/) {
				if (filename == "") {
					auto result = ofSystemLoadDialog("Select CSV");
					if (!result.bSuccess) {
						return;
					}
					filename = result.filePath;
				}
				ofFile file(filename, ofFile::Mode::WriteOnly, false);
				if (!file.exists()) {
					throw(ofxRulr::Exception("File not found"));
				}
			}

			//----------
			//https://github.com/opencv/opencv/blob/master/samples/cpp/tutorial_code/calib3d/real_time_pose_estimation/src/main_detection.cpp
			void Body::updateTracking(cv::Mat rotationVector, cv::Mat translation) {
				if (this->parameters.kalmanFilter.enabled) {
					this->kalmanFilterMutex.lock();
					auto kalmanFilter = this->kalmanFilter;
					this->kalmanFilterMutex.unlock();

					//create the measurement vector
					cv::Mat measurement;
					cv::vconcat(translation, rotationVector, measurement);

					//make a Kalman filter if ours is invalidated
					if (!kalmanFilter) {
						kalmanFilter = Body::makeKalmanFilter();

						this->kalmanFilterMutex.lock();
						this->kalmanFilter = kalmanFilter;
						this->kalmanFilterMutex.unlock();

						kalmanFilter->statePre = measurement;
					}

					//predict and correct
					auto prediction = kalmanFilter->predict();
					auto estimation = kalmanFilter->correct(measurement);

					translation.at<double>(0) = estimation.at<double>(0);
					translation.at<double>(1) = estimation.at<double>(1);
					translation.at<double>(2) = estimation.at<double>(2);

					rotationVector.at<double>(0) = estimation.at<double>(9);
					rotationVector.at<double>(1) = estimation.at<double>(10);
					rotationVector.at<double>(2) = estimation.at<double>(11);
				}

				transformIncoming.send(ofxCv::makeMatrix(rotationVector, translation));
			}

			//----------
			void Body::invalidateBodyDescription() {
				auto lock = unique_lock<mutex>(this->bodyDescriptionMutex);
				this->bodyDescription.reset();
			}

			//----------
			shared_ptr<cv::KalmanFilter> Body::makeKalmanFilter() {
				auto kalmanFilter = make_shared<cv::KalmanFilter>();
				kalmanFilter->init(18, 6, 0, CV_64F);

				cv::setIdentity(kalmanFilter->processNoiseCov, cv::Scalar::all(1e-5));
				cv::setIdentity(kalmanFilter->measurementNoiseCov, cv::Scalar::all(1e-2));
				cv::setIdentity(kalmanFilter->errorCovPost, cv::Scalar::all(1));

				/** DYNAMIC MODEL **/

				//  [1 0 0 dt  0  0 dt2   0   0 0 0 0  0  0  0   0   0   0]
				//  [0 1 0  0 dt  0   0 dt2   0 0 0 0  0  0  0   0   0   0]
				//  [0 0 1  0  0 dt   0   0 dt2 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  1  0  0  dt   0   0 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  0  1  0   0  dt   0 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  0  0  1   0   0  dt 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  0  0  0   1   0   0 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  0  0  0   0   1   0 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  0  0  0   0   0   1 0 0 0  0  0  0   0   0   0]
				//  [0 0 0  0  0  0   0   0   0 1 0 0 dt  0  0 dt2   0   0]
				//  [0 0 0  0  0  0   0   0   0 0 1 0  0 dt  0   0 dt2   0]
				//  [0 0 0  0  0  0   0   0   0 0 0 1  0  0 dt   0   0 dt2]
				//  [0 0 0  0  0  0   0   0   0 0 0 0  1  0  0  dt   0   0]
				//  [0 0 0  0  0  0   0   0   0 0 0 0  0  1  0   0  dt   0]
				//  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  1   0   0  dt]
				//  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  0   1   0   0]
				//  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  0   0   1   0]
				//  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  0   0   0   1]

				// TODO : how to deal with varying dt?
				auto dt = 1.0f / 120.0f;
				auto dt2 = dt * dt;

				// position
				kalmanFilter->transitionMatrix.at<double>(0, 3) = dt;
				kalmanFilter->transitionMatrix.at<double>(1, 4) = dt;
				kalmanFilter->transitionMatrix.at<double>(2, 5) = dt;
				kalmanFilter->transitionMatrix.at<double>(3, 6) = dt;
				kalmanFilter->transitionMatrix.at<double>(4, 7) = dt;
				kalmanFilter->transitionMatrix.at<double>(5, 8) = dt;
				kalmanFilter->transitionMatrix.at<double>(0, 6) = 0.5 * dt2;
				kalmanFilter->transitionMatrix.at<double>(1, 7) = 0.5 * dt2;
				kalmanFilter->transitionMatrix.at<double>(2, 8) = 0.5 * dt2;

				// orientation
				kalmanFilter->transitionMatrix.at<double>(9, 12) = dt;
				kalmanFilter->transitionMatrix.at<double>(10, 13) = dt;
				kalmanFilter->transitionMatrix.at<double>(11, 14) = dt;
				kalmanFilter->transitionMatrix.at<double>(12, 15) = dt;
				kalmanFilter->transitionMatrix.at<double>(13, 16) = dt;
				kalmanFilter->transitionMatrix.at<double>(14, 17) = dt;
				kalmanFilter->transitionMatrix.at<double>(9, 15) = 0.5 * dt2;
				kalmanFilter->transitionMatrix.at<double>(10, 16) = 0.5 * dt2;
				kalmanFilter->transitionMatrix.at<double>(11, 17) = 0.5 * dt2;

				/** MEASUREMENT MODEL **/

				//  [1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
				//  [0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
				//  [0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
				//  [0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0]
				//  [0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0]
				//  [0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0]
				kalmanFilter->measurementMatrix.at<double>(0, 0) = 1;  // x
				kalmanFilter->measurementMatrix.at<double>(1, 1) = 1;  // y
				kalmanFilter->measurementMatrix.at<double>(2, 2) = 1;  // z
				kalmanFilter->measurementMatrix.at<double>(3, 9) = 1;  // roll
				kalmanFilter->measurementMatrix.at<double>(4, 10) = 1; // pitch
				kalmanFilter->measurementMatrix.at<double>(5, 11) = 1; // yaw

				return kalmanFilter;
			}
		}
	}
}
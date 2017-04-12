#include "pch_Plugin_MoCap.h"
#include "Body.h"
#include "ofxGLM.h"

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
				ss << "ID : " << this->ID.get() << endl;
				ss << ofToString(this->position.get(), 3) << endl;
				return ss.str();
			}

			//----------
			void Body::Marker::serialize(Json::Value & json) {
				Utils::Serializable::serialize(json, this->ID);
				Utils::Serializable::serialize(json, this->position);
			}

			//----------
			void Body::Marker::deserialize(const Json::Value & json) {
				Utils::Serializable::deserialize(json, this->ID);
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

				this->markers.onSelectionChanged += [this]() {
					this->invalidateBodyDescription();
				};

				auto panel = ofxCvGui::Panels::makeWorld();
				panel->setGridEnabled(false);
				panel->onDrawWorld += [this](ofCamera & camera) {
					auto bodyDescription = this->getBodyDescription();
					if (!bodyDescription) {
						return;
					}

					ofVec3f boundsMin;
					ofVec3f boundsMax;

					float maxDistanceSquared = 0.0f;

					//draw marker spheres
					{
						ofPushStyle();
						{
							for (int i = 0; i < bodyDescription->markerCount; i++) {
								const auto & position = bodyDescription->markers.positions[i];
								ofSetColor(bodyDescription->markers.colors[i]);
								ofDrawSphere(position, bodyDescription->markerDiameter / 2.0f);
								ofDrawBitmapString(bodyDescription->markers.IDs[i], bodyDescription->markers.positions[i]);

								boundsMin.x = MIN(boundsMin.x, position.x);
								boundsMin.y = MIN(boundsMin.y, position.y);
								boundsMin.z = MIN(boundsMin.z, position.z);
								boundsMax.x = MAX(boundsMin.x, position.x);
								boundsMax.y = MAX(boundsMin.y, position.y);
								boundsMax.z = MAX(boundsMin.z, position.z);

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
							gridSize = MAX(abs(boundsMax.z), abs(boundsMin.z), abs(boundsMax.x), abs(boundsMin.x));
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
						bodyDescription->markers.IDs.push_back(markers[i]->ID);
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
						auto maxSize = ofGetFrameRate() * this->parameters.drawStyleWorld.historyTrailLength;
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

				if (this->parameters.drawStyleWorld.useColors) {
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

				if (this->parameters.drawStyleWorld.showIDs) {
					for (const auto & marker : markers) {
						ofDrawBitmapString(ofToString(marker->ID), marker->position);;
					}
				}
			}

			//----------
			void Body::drawWorld() const {
				auto selection = this->markers.getSelection();

				//the trails are in world space so we treat these outside of drawObject
				ofPushStyle();
				{
					ofEnableAlphaBlending();
					for (auto marker : selection) {
						ofMesh trail;
						trail.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);
						ofColor color = this->parameters.drawStyleWorld.useColors.get() ? marker->color : ofColor(255);

						int tailIndex = 0;
						auto historyLength = marker->worldHistory.size();
						for (auto point : marker->worldHistory) {
							trail.addVertex(point);
							color.a = ofMap(tailIndex++
								, 0, (float)historyLength
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
					try {
						this->addMarker();
					}
					RULR_CATCH_ALL_TO_ALERT;
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
					auto csvRow = ofSplitString(line, ", ", true, true);
					MarkerID ID;
					if (csvRow.size() == 4) {
						// X, Y, Z, ID
						ID = ofToInt(csvRow[3]);
					}
					else if (csvRow.size() == 3) {
						// X, Y, Z
						ID = this->getNextAvailableID();
					}
					else {
						// Unknown format
						continue;
					}

					ofVec3f position(ofToFloat(csvRow[0])
						, ofToFloat(csvRow[1])
						, ofToFloat(csvRow[2]));

					auto marker = make_shared<Marker>();
					marker->position = position;
					marker->ID = ID;
					this->markers.add(marker);
				}

				this->invalidateBodyDescription();
			}

			//----------
			void Body::saveCSV(string filename /*= ""*/) {
				if (filename == "") {
					auto result = ofSystemSaveDialog("marker.csv", "Select CSV");
					if (!result.bSuccess) {
						return;
					}
					filename = result.filePath;
				}
				ofstream file(filename, ios::out | ios::trunc);
				auto markers = this->markers.getSelection();
				for (auto marker : markers) {
					file << marker->ID << ", "
						<< marker->position.get().x << ", "
						<< marker->position.get().y << ", "
						<< marker->position.get().z << endl;
				}
				file.close();
			}

			//----------
			void Body::addMarker() {
				auto inputText = ofSystemTextBoxDialog("Marker position [m] 'x, y, z'");
				if (!inputText.empty()) {
					stringstream inputTextStream(inputText);
					ofVec3f position;
					inputTextStream >> position;

					{
						auto marker = make_shared<Marker>();
						marker->ID = this->getNextAvailableID();
						marker->position = position;
						this->markers.add(marker);
					}

					this->invalidateBodyDescription();
				}
			}

			//----------
			void Body::addMarker(const ofVec3f & position) {
				auto marker = make_shared<Marker>();
				marker->ID = this->getNextAvailableID();
				marker->position = position;
				this->markers.add(marker);
			}

			//----------
			MarkerID Body::getNextAvailableID() const {
				auto markers = this->markers.getAllCaptures();
				MarkerID ID = 0;
				for (const auto & marker : markers) {
					if (marker->ID >= ID) {
						ID = marker->ID + 1;
					}
				}
				return ID;
			}

			//----------
			void Body::invalidateBodyDescription() {
				auto lock = unique_lock<mutex>(this->bodyDescriptionMutex);
				this->bodyDescription.reset();
			}
		}
	}
}
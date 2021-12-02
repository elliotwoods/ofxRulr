#include "pch_Plugin_ArUco.h"
#include "MarkerMap.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
			//----------
			MarkerMap::MarkerMap() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			string MarkerMap::getTypeName() const {
				return "ArUco::MarkerMap";
			}

			//----------
			void MarkerMap::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<Detector>("Detector for preview");
				this->manageParameters(this->parameters);
			}

			//----------
			void MarkerMap::drawWorldStage() {
				//get the detector for use previewing the markers
				auto detectorNode = this->getInput<Detector>();

				if (this->markerMap) {
					//build a mesh to use for previews 
					ofMesh markerPreview;
					{
						markerPreview.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLE_FAN);
						markerPreview.addTexCoords({
							{ 0, 0 }
							,{ ARUCO_PREVIEW_RESOLUTION, 0 }
							,{ ARUCO_PREVIEW_RESOLUTION, ARUCO_PREVIEW_RESOLUTION }
							,{ 0, ARUCO_PREVIEW_RESOLUTION }
						});
					}

					//cycle through markers and draw them
					const auto & markerMap = *this->markerMap;
					for (const auto & marker3D : markerMap) {
						markerPreview.clearVertices();

						for (auto & vertex : marker3D.points) {
							markerPreview.addVertex(ofxCv::toOf(vertex));
						}

						if (detectorNode) {
							auto & image = detectorNode->getMarkerImage(marker3D.id);

							image.bind();
							{
								markerPreview.draw();
							}
							image.unbind();
						}
						else {
							markerPreview.draw();
						}

						if (isActive(this, this->parameters.drawLabels.get())) {
							ofxCvGui::Utils::drawTextAnnotation(ofToString(marker3D.id)
								, ofxCv::toOf(marker3D[0])
								, this->getColor());
						}
					}
				}
			}

			//----------
			void MarkerMap::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Import...", [this]() {
					try {
						auto result = ofSystemLoadDialog("Select marker map YML");
						if (result.bSuccess) {
							auto markerMap = make_shared<aruco::MarkerMap>(result.filePath);
							if (markerMap->empty()) {
								throw(ofxRulr::Exception("Failed to load marker map"));
							}
							this->markerMap = move(markerMap);
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
				inspector->addButton("Clear", [this]() {
					this->clear();
				});

				{
					inspector->addTitle("Rotate 90", ofxCvGui::Widgets::Title::Level::H3);
					inspector->addButton("X", [this]() {
						this->rotateMarkerMap(ofVec3f(1, 0, 0), 90);
					});
					inspector->addButton("Y", [this]() {
						this->rotateMarkerMap(ofVec3f(0, 1, 0), 90);
					});
					inspector->addButton("Z", [this]() {
						this->rotateMarkerMap(ofVec3f(0, 0, 1), 90);
					});
					inspector->addButton("Remove...", [this]() {
						auto removeString = ofSystemTextBoxDialog("Marker index");
						if (!removeString.empty()) {
							auto index = ofToInt(removeString);
							this->removeMarker(index);
						}
					});
				}
			}

			//----------
			void MarkerMap::serialize(nlohmann::json & json) {
				if (this->markerMap) {
					auto & jsonMarkerMap = json["markerMap"];
					jsonMarkerMap["mInfoType"] = this->markerMap->mInfoType;
					jsonMarkerMap["dictionary"] = this->markerMap->getDictionary();
					auto & jsonMarkers = jsonMarkerMap["markers"];
					for (const auto & marker : *this->markerMap) {
						nlohmann::json jsonMarker;
						jsonMarker["id"] = marker.id;
						auto & jsonPoints = jsonMarker["points"];
						for (const auto & point : marker.points) {
							nlohmann::json jsonPoint;
							Utils::serialize(jsonPoint, ofxCv::toOf(point));
							jsonPoints.push_back(jsonPoint);
						}
						jsonMarkers.push_back(jsonMarker);
					}
				}
			}

			//----------
			void MarkerMap::deserialize(const nlohmann::json & json) {
				if (json.contains("markerMap")) {
					auto markerMap = make_shared<aruco::MarkerMap>();
					const auto & jsonMarkerMap = json["markerMap"];
					{
						int value;
						if (Utils::deserialize(jsonMarkerMap["mInfoType"], value)) {
							markerMap->mInfoType = value;
						}
					}
					{
						std::string value;
						if (Utils::deserialize(jsonMarkerMap["dictionary"], value)) {
							markerMap->setDictionary(value);
						}
					}
					const auto & jsonMarkers = jsonMarkerMap["markers"];
					for (const auto & jsonMarker : jsonMarkers) {
						aruco::Marker3DInfo marker;
						marker.id = jsonMarker["id"].get<int>();
						const auto & jsonPoints = jsonMarker["points"];
						for (const auto & jsonPoint : jsonPoints) {
							glm::vec3 point;
							Utils::deserialize(jsonPoint, point);
							marker.push_back(ofxCv::toCv(point));
						}
						markerMap->push_back(marker);
					}
					this->markerMap = markerMap;
				}
			}

			//----------
			shared_ptr<aruco::MarkerMap> MarkerMap::getMarkerMap() {
				return this->markerMap;
			}

			//----------
			void MarkerMap::clear() {
				this->markerMap.reset();
			}

			//----------
			void MarkerMap::rotateMarkerMap(const ofVec3f & axis, float angle) {
				if (this->markerMap) {
					auto transform = ofMatrix4x4::newRotationMatrix(angle, axis);
					for (auto & marker : *this->markerMap) {
						for (auto & point : marker.points) {
							point = ofxCv::toCv(Utils::applyTransform(transform, ofxCv::toOf(point)));
						}
					}
				}
			}

			//----------
			void MarkerMap::removeMarker(size_t idToRemove) {
				if (this->markerMap) {
					auto toRemove = remove_if(this->markerMap->begin()
						, this->markerMap->end()
						, [idToRemove](const aruco::Marker3DInfo & markerInfo) {
						return markerInfo.id == idToRemove;
					});
					this->markerMap->erase(toRemove, this->markerMap->end());
				}
			}
		}
	}
}
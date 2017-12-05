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

						switch (this->parameters.drawLabels.get().get()) {
						case WhenDrawOnWorldStage::Selected:
							if (!this->isBeingInspected()) {
								break;
							}
						case WhenDrawOnWorldStage::Always:
							ofDrawBitmapString(ofToString(marker3D.id), ofxCv::toOf(marker3D[0]));
							break;
						case WhenDrawOnWorldStage::Never:
						default:
							break;
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
			}

			//----------
			void MarkerMap::serialize(Json::Value & json) {
				if(this->markerMap) {
					auto & jsonMarkerMap = json["markerMap"];
					jsonMarkerMap["mInfoType"] = this->markerMap->mInfoType;
					jsonMarkerMap["dictionary"] = this->markerMap->getDictionary();
					auto & jsonMarkers = jsonMarkerMap["markers"];
					for (const auto & marker : *this->markerMap) {
						Json::Value jsonMarker;
						jsonMarker["id"] = marker.id;
						auto & jsonPoints = jsonMarker["points"];
						for (const auto & point : marker.points) {
							Json::Value jsonPoint;
							jsonPoint.append(point.x);
							jsonPoint.append(point.y);
							jsonPoint.append(point.z);
							jsonPoints.append(jsonPoint);
						}
						jsonMarkers.append(jsonMarker);
					}
				}
			}

			//----------
			void MarkerMap::deserialize(const Json::Value & json) {
				if (json.isMember("markerMap")) {
					auto markerMap = make_shared<aruco::MarkerMap>();
					const auto & jsonMarkerMap = json["markerMap"];
					markerMap->mInfoType = jsonMarkerMap["mInfoType"].asInt();
					markerMap->setDictionary(jsonMarkerMap["dictionary"].asString());
					const auto & jsonMarkers = jsonMarkerMap["markers"];
					for (const auto & jsonMarker : jsonMarkers) {
						aruco::Marker3DInfo marker;
						marker.id = jsonMarker["id"].asInt();
						const auto & jsonPoints = jsonMarker["points"];
						for (const auto & jsonPoint : jsonPoints) {
							cv::Point3f point;
							point.x = jsonPoint[0].asFloat();
							point.y = jsonPoint[1].asFloat();
							point.z = jsonPoint[2].asFloat();
							marker.push_back(point);
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
		}
	}
}
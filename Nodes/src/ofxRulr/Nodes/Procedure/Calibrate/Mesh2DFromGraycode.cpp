#include "pch_RulrNodes.h"
#include "Mesh2DFromGraycode.h"

#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"

#include "ofxTriangle.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				Mesh2DFromGraycode::Mesh2DFromGraycode() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string Mesh2DFromGraycode::getTypeName() const {
					return "Nodes::Procedure::Calibrate::MeshFrom2DGraycode";
				}

				//----------
				void Mesh2DFromGraycode::init() {
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					auto graycodeInput = this->addInput<Scan::Graycode>();

					auto view = make_shared<ofxCvGui::Panels::Draws>();
					view->onDrawCropped += [this](ofxCvGui::Panels::BaseImage::DrawCroppedArguments & args) {
						this->mesh.drawWireframe();
					};
					graycodeInput->onNewConnection += [this, view](shared_ptr<Scan::Graycode> graycodeNode) {
						view->setDrawObject(graycodeNode->getDecoder().getCameraInProjector());
					};
					graycodeInput->onDeleteConnection += [this, view](shared_ptr<Scan::Graycode> graycodeNode) {
						view->clearDrawObject();
					};
					this->view = view;
				}

				//----------
				ofxCvGui::PanelPtr Mesh2DFromGraycode::getView() {
					return this->view;
				}

				//----------
				void Mesh2DFromGraycode::get(ofMesh & mesh) const {
					mesh = this->mesh;
				}

				//----------
				void Mesh2DFromGraycode::populateInspector(ofxCvGui::InspectArguments & args) {
					auto inspector = args.inspector;

					inspector->addButton("Triangulate", [this]() {
						try {
							this->triangulate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					});
				}

				//----------
				void Mesh2DFromGraycode::serialize(Json::Value & json) {
				}

				//----------
				void Mesh2DFromGraycode::deserialize(const Json::Value & json) {
				}

				//----------
				//from http://flassari.is/2008/11/line-line-intersection-in-cplusplus/
				ofVec2f * line_line_intersection(ofVec2f p1, ofVec2f p2, ofVec2f p3, ofVec2f p4) {

					// Store the values for fast access and easy
					// equations-to-code conversion
					float x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
					float y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;

					float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
					// If d is zero, there is no intersection
					if (d == 0) return NULL;

					// Get the x and y
					float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
					float x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
					float y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

					// Check if the x and y coordinates are within both lines
					if (x < min(x1, x2) || x > max(x1, x2) ||
						x < min(x3, x4) || x > max(x3, x4)) return NULL;
					if (y < min(y1, y2) || y > max(y1, y2) ||
						y < min(y3, y4) || y > max(y3, y4)) return NULL;

					// Return the point of intersection
					ofVec2f * ret = new ofVec2f();
					ret->x = x;
					ret->y = y;
					return ret;
				}

				//----------
				void Mesh2DFromGraycode::triangulate() {
					this->throwIfMissingAnyConnection();

					auto dataSet = this->getInput<Scan::Graycode>()->getDataSet();
					if (!dataSet.getHasData()) {
						throw(Exception("No scan data available"));
					}

					//get camera coords in projector space map
					const auto & cameraInProjector = dataSet.getDataInverse();
					const auto & active = dataSet.getActive();

					const auto projectorWidth = cameraInProjector.getWidth();
					const auto projectorHeight = cameraInProjector.getHeight();

					const auto cameraWidth = active.getWidth();

					auto getCameraPixelPosition = [&cameraInProjector, projectorWidth, cameraWidth](int i, int j) {
						const auto cameraPixelIndex = cameraInProjector.getData()[i + j * projectorWidth];
						return ofVec2f(cameraPixelIndex % cameraWidth, cameraPixelIndex / cameraWidth);
					};
					auto isActive = [& cameraInProjector, &active, projectorWidth](int i, int j) {
						const auto cameraPixelIndex = cameraInProjector.getData()[i + j * projectorWidth];
						const auto isActive = active.getData()[cameraPixelIndex];
						return isActive;
					};

					vector<ofPoint> projectorSpaceActivePoints;

					//find all active pixels
					for (int j = 0; j < projectorHeight; j++) {
						for (int i = 0; i < projectorWidth; i++) {
							if (isActive(i, j)) {
								projectorSpaceActivePoints.emplace_back(ofVec2f(i, j));
							}
						}
					}

					//triangulate
					ofMesh mesh;
					{
						//make vertices and tex coords
						Delaunay::Point tempP;
						vector<Delaunay::Point> delauneyPoints;
						for (const auto & projectorSpaceActivePoint : projectorSpaceActivePoints) {
							delauneyPoints.emplace_back(projectorSpaceActivePoint.x, projectorSpaceActivePoint.y);
							mesh.addVertex(projectorSpaceActivePoint);
							mesh.addTexCoord(getCameraPixelPosition(projectorSpaceActivePoint.x, projectorSpaceActivePoint.y));
						}

						//triangulate
						auto delauney = make_shared<Delaunay>(delauneyPoints);
						delauney->Triangulate();

						//apply indices
						for (auto it = delauney->fbegin(); it != delauney->fend(); ++it) {
							mesh.addIndex(delauney->Org(it));
							mesh.addIndex(delauney->Dest(it));
							mesh.addIndex(delauney->Apex(it));
						}
					}
					swap(this->mesh, mesh);
				}
			}
		}
	}
}

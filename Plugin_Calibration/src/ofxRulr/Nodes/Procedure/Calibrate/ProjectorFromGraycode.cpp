#include "pch_Plugin_Calibration.h"
#include "ProjectorFromGraycode.h"

#include "ofxRulr/Nodes/Procedure/Scan/Graycode.h"
#include "ofxRulr/Nodes/Item/Camera.h"
#include "ofxRulr/Nodes/Item/Projector.h"
#include "ofxRulr/Nodes/Item/AbstractBoard.h"

#include "ofxTriangle.h"

using namespace ofxCv;

namespace ofxRulr {
	namespace Nodes {
		namespace Procedure {
			namespace Calibrate {
				//----------
				ProjectorFromGraycode::ProjectorFromGraycode() {
					RULR_NODE_INIT_LISTENER;
				}

				//----------
				string ProjectorFromGraycode::getTypeName() const {
					return "Procedure::Calibrate::ProjectorFromGraycode";
				}

				//----------
				ofxCvGui::PanelPtr ProjectorFromGraycode::getPanel() {
					return this->panel;
				}

				//----------
				void ProjectorFromGraycode::init() {
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					this->addInput<Item::Projector>();
					this->addInput<Item::Camera>();
					this->addInput<Item::AbstractBoard>();
					this->addInput<Scan::Graycode>();

					this->panel = ofxCvGui::Panels::makeTexture(this->preview.getTexture());
				}

				//----------
				void ProjectorFromGraycode::drawWorldStage() {
					ofMesh preview;
					preview.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

					auto projectorNode = this->getInput<Item::Projector>();

					bool useColor = false;
					float projectorWidth, projectorHeight;
					if (projectorNode) {
						useColor = true;
						projectorWidth = projectorNode->getWidth();
						projectorHeight = projectorNode->getHeight();
					}

					auto selection = this->getSelection();

					for (auto index : selection) {
						ofMesh line;
						auto & capture = this->captures[index];
						line.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);

						for (int i = 0; i < capture.worldPoints.size(); i++) {
							const auto & worldPoint = capture.worldPoints[i];
							if (i == 0) {
								ofDrawBitmapString(ofToString(index), worldPoint);
							}
							preview.addVertex(worldPoint);
							line.addVertex(worldPoint);
							line.addColor((float)i / (float)capture.worldPoints.size());
						}

						for (auto & projectorImagePoint : capture.projectorImagePoints) {
							if (useColor) {
								preview.addColor(ofColor(
									ofMap(projectorImagePoint.x, 0, projectorWidth, 0, 255),
									ofMap(projectorImagePoint.y, 0, projectorHeight, 0, 255),
									0));
							}
						}

						line.draw();
					}

					Utils::Graphics::pushPointSize(5.0f);
					{
						preview.drawVertices();
					}
					Utils::Graphics::popPointSize();
				}

				//----------
				//from http://stackoverflow.com/questions/13299409/how-to-get-the-image-pixel-at-real-locations-in-opencv#
				template<typename Type> // Type could be Vec3b for RGB8
				Type getColorSubpix(const cv::Mat& img, cv::Point2f pt)
				{
					cv::Mat patch;
					cv::remap(img, patch, cv::Mat(1, 1, CV_32FC2, &pt), cv::noArray(),
						cv::INTER_LINEAR, cv::BORDER_REFLECT_101);
					return patch.at<Type>(0, 0);
				}

				//----------
				void ProjectorFromGraycode::addCapture() {
					Utils::ScopedProcess scopedProcess("Add capture");

					this->throwIfMissingAnyConnection();
					auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();
					auto cameraNode = this->getInput<Item::Camera>();
					auto boardNode = this->getInput<Item::AbstractBoard>();
					auto projectorNode = this->getInput<Item::Projector>();
					
					if (this->parameters.capture.autoScan) {
						graycodeNode->runScan();
					}

					auto & dataSet = graycodeNode->getDataSet();
					if (!dataSet.getHasData()) {
						throw(ofxRulr::Exception("Graycode node has no data"));
					}

					projectorNode->setWidth(dataSet.getPayloadWidth());
					projectorNode->setHeight(dataSet.getPayloadHeight());

					//Find board in camera
					vector<cv::Point2f> pointsInCameraImage;
					vector<cv::Point3f> boardObjectPoints;
					{
						Utils::ScopedProcess scopedProcessFindBoard("Find board in camera image", false);
						const auto & median = dataSet.getMedian();

						ofPixels medianCopy(median);
						auto medianCopyMat = toCv(medianCopy);

						if (this->parameters.capture.searchBrightArea) {
							vector<vector<cv::Point>> contours;

							//crop the image down to active area
							cv::Rect bounds;
							cv::Mat croppedImage;
							{
								//threshold the image
								cv::Mat thresholded;
								cv::threshold(medianCopyMat, thresholded, this->parameters.capture.brightAreaThreshold, 255, THRESH_BINARY);

								//find contours
								cv::findContours(thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

								//add all contours into one set of points
								vector<cv::Point> contoursCollapsed;
								for (auto & contour : contours) {
									auto contourBounds = cv::boundingRect(cv::Mat(contour));
									if (contourBounds.area() < 25) {
										continue;
									}
									for (auto & point : contour) {
										contoursCollapsed.push_back(point);
									}
								}
								if (contoursCollapsed.size() > 0) {
									bounds = cv::boundingRect(cv::Mat(contoursCollapsed));
									croppedImage = medianCopyMat(bounds);
								}
								else {
									croppedImage = medianCopyMat;
								}
							}

							ofxCv::copy(croppedImage, this->preview.getPixels());
							this->preview.update();

							//find checkerboard in cropped image
							if (!boardNode->findBoard(croppedImage
								, pointsInCameraImage
								, boardObjectPoints
								, this->parameters.capture.findBoardMode
								, cameraNode->getCameraMatrix()
								, cameraNode->getDistortionCoefficients())) {
								throw(ofxRulr::Exception("Board not found in camera image"));
							}

							//relocate points back into non-cropped space
							for (auto & point : pointsInCameraImage) {
								point.x += bounds.x;
								point.y += bounds.y;
							}
						}
						else {
							ofxCv::copy(medianCopyMat, this->preview.getPixels());
							this->preview.update();

							if (!boardNode->findBoard(medianCopyMat
								, pointsInCameraImage
								, boardObjectPoints
								, this->parameters.capture.findBoardMode
								, cameraNode->getCameraMatrix()
								, cameraNode->getDistortionCoefficients())) {
								throw(ofxRulr::Exception("Board not found in camera image"));
							}
						}
					}
					
					//find pose of board in 3D space
					ofMatrix4x4 boardTransform;
					{
						Mat rotation, translation;

						if (this->parameters.capture.useRansacForSolvePnp) {
							cv::solvePnPRansac(boardObjectPoints
								, pointsInCameraImage
								, cameraNode->getCameraMatrix()
								, cameraNode->getDistortionCoefficients()
								, rotation
								, translation
								, false
								, 1.0f
								, boardObjectPoints.size() / 2 * 3);
						}
						else {
							cv::solvePnP(boardObjectPoints
								, pointsInCameraImage
								, cameraNode->getCameraMatrix()
								, cameraNode->getDistortionCoefficients()
								, rotation
								, translation);
						}
						
						boardTransform = makeMatrix(rotation, translation);
					}
					
					//find 3d points
					vector<ofVec3f> boardPointsInWorldSpace;
					{
						for (auto & boardObjectPoint : boardObjectPoints) {
							boardPointsInWorldSpace.push_back(toOf(boardObjectPoint) * boardTransform * cameraNode->getTransform());
						}
					}

					//build a look up mesh for projector coords within camera image
					ofMesh projectorInCameraMesh;
					{
						Utils::ScopedProcess scopedProcessBuildMesh("Build delauney mesh", false);
						auto & projectorInCameraPreview = graycodeNode->getDecoder().getProjectorInCamera().getPixels();

						auto activeEroded = dataSet.getActive();
						auto activeErodedMat = toCv(activeEroded);
						{
							auto boardBounds = cv::boundingRect(Mat(pointsInCameraImage));
							ofVec2f boardSizeInCamera(boardBounds.width, boardBounds.height);
							float erosionSize = boardSizeInCamera.length() * this->parameters.capture.erosion;
							cv::erode(activeErodedMat, activeErodedMat, cv::Mat(), cv::Point(-1, -1), (int)erosionSize);
						}

						//show preview of eroded
						{
							Mat copy;
							toCv(projectorInCameraPreview).copyTo(copy, activeErodedMat);
							ofxCv::copy(copy, this->preview.getPixels());
							this->preview.update();
						}
						


						//make vertices and tex coords
						Delaunay::Point tempP;
						vector<Delaunay::Point> delauneyPoints;
						auto activeErodedPixel = activeEroded.getData();
						for (auto & pixel : dataSet) {
							if (!*activeErodedPixel++) {
								continue;
							}

							auto projectorXY = pixel.getProjectorXY();
							auto cameraXY = pixel.getCameraXY();

							delauneyPoints.emplace_back(cameraXY.x, cameraXY.y);
							projectorInCameraMesh.addVertex(cameraXY);
							projectorInCameraMesh.addColor(ofFloatColor(projectorXY.x, projectorXY.y, 0));
						}

						//triangulate
						auto delauney = make_shared<Delaunay>(delauneyPoints);
						delauney->Triangulate();

						//apply indices
						for (auto it = delauney->fbegin(); it != delauney->fend(); ++it) {
							//cut off big triangles
							if (delauney->area(it) > this->parameters.capture.maxTriangleArea) {
								continue;
							}
							projectorInCameraMesh.addIndex(delauney->Org(it));
							projectorInCameraMesh.addIndex(delauney->Dest(it));
							projectorInCameraMesh.addIndex(delauney->Apex(it));
						}
					}

					//draw the look up mesh into an fbo
					ofFbo projectorInCameraFbo;
					{
						Utils::ScopedProcess scopedProcessBuildFbo("Build fbo", false);

						{
							ofFbo::Settings fboSettings;
							fboSettings.width = cameraNode->getWidth();
							fboSettings.height = cameraNode->getHeight();
							fboSettings.internalformat = GL_RGBA32F;
							projectorInCameraFbo.allocate(fboSettings);
						}

						projectorInCameraFbo.begin();
						{
							ofClear(0, 255);
							projectorInCameraMesh.draw();
						}
						projectorInCameraFbo.end();
					}

					//read back to pixels;
					ofFloatPixels projectorInCameraPixels;
					{
						//pre-allocate to dodge memory error
						projectorInCameraPixels.allocate(cameraNode->getWidth(), cameraNode->getHeight(), ofPixelFormat::OF_PIXELS_RGBA);

						Utils::ScopedProcess scopedProcessBuildFbo("Read back fbo", false);
						projectorInCameraFbo.readToPixels(projectorInCameraPixels);
					}

					//read all projector pixel positions and add working points into capture
					Capture capture;
					{
						auto worldPoint = boardPointsInWorldSpace.begin();

						for (auto & cameraPoint : pointsInCameraImage) {
							auto projectorPoint = getColorSubpix<cv::Vec2f>(toCv(projectorInCameraPixels), cameraPoint);

							if (projectorPoint[0] == 0 && projectorPoint[1] == 0) {
								//skip projector and world points if we can't find the proj coord for this corner
								worldPoint++;
								continue;
							}

							capture.projectorImagePoints.push_back(toOf((cv::Point2f) projectorPoint));
							capture.worldPoints.push_back(*worldPoint++);
						}
					}

					this->captures.push_back(capture);

					scopedProcess.end();
				}

				//----------
				void ProjectorFromGraycode::deleteLastCapture() {
					if (!this->captures.empty()) {
						this->captures.pop_back();
					}
					this->preview.clear();
				}

				//----------
				void ProjectorFromGraycode::clearCaptures() {
					this->captures.clear();
					this->preview.clear();
				}

				//----------
				void ProjectorFromGraycode::calibrate() {
					Utils::ScopedProcess scopedProcess("Calibrate projector");
					
					this->throwIfMissingAConnection<Item::Projector>();
					auto projectorNode = this->getInput<Item::Projector>();

					auto projectorWidth = projectorNode->getWidth();
					auto projectorHeight = projectorNode->getHeight();

					vector<ofVec3f> worldPoints;
					vector<ofVec2f> projectorImagePoints;

					auto selection = this->getSelection();
					for (auto index : selection) {
						auto & capture = this->captures[index];

						worldPoints.insert(worldPoints.end()
							, capture.worldPoints.begin()
							, capture.worldPoints.end());
						projectorImagePoints.insert(projectorImagePoints.end()
							, capture.projectorImagePoints.begin()
							, capture.projectorImagePoints.end());
					}
					cv::Mat cameraMatrix, rotation, translation;
					this->error = ofxCv::calibrateProjector(cameraMatrix, rotation, translation,
						worldPoints, projectorImagePoints,
						this->getInput<Item::Projector>()->getWidth(), this->getInput<Item::Projector>()->getHeight(),
						false,
						0.5f, 1.4f);

					auto view = ofxCv::makeMatrix(rotation, translation);
					projectorNode->setTransform(view.getInverse());
					projectorNode->setIntrinsics(cameraMatrix);

					scopedProcess.end();
				}

				//----------
				void ProjectorFromGraycode::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
					auto inspector = inspectArgs.inspector;

					inspector->addButton("Add capture", [this]() {
						try {
							this->addCapture();
						}
						RULR_CATCH_ALL_TO_ERROR;
					}, ' ');
					inspector->addButton("Delete last capture", [this]() {
						this->deleteLastCapture();
					});
					inspector->addButton("Delete selection", [this]() {
						this->deleteSelection();
					});
					inspector->addButton("Clear captures", [this]() {
						this->clearCaptures();
					});
					inspector->addLiveValue<size_t>("Capture count", [this]() {
						return this->captures.size();
					});
					inspector->addButton("Calibrate", [this]() {
						try {
							this->calibrate();
						}
						RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
					inspector->addLiveValue<float>("Reprojection error", [this]() {
						return this->error;
					});
					inspector->addParameterGroup(this->parameters);
				}

				//----------
				void ProjectorFromGraycode::serialize(Json::Value & json) {
					Utils::Serializable::serialize(json, this->parameters);

					auto & jsonCaptures = json["captures"];
					for (int i = 0; i < this->captures.size(); i++) {
						auto & capture = this->captures[i];
						auto & jsonCapture = jsonCaptures[i];
						
						for (int j = 0; j < capture.worldPoints.size(); j++) {
							auto & jsonCorrespondence = jsonCapture[j];
							jsonCorrespondence["worldPoint"] << capture.worldPoints[j];
							jsonCorrespondence["projectorImagePoint"] << capture.projectorImagePoints[j];
						}
					}
				}

				//----------
				void ProjectorFromGraycode::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);

					this->captures.clear();
					const auto & jsonCaptures = json["captures"];
					for (const auto & jsonCapture : jsonCaptures) {
						Capture capture;
						int pointIndex = 0;
						for (const auto & jsonCorrespondence : jsonCapture) {
							ofVec3f worldPoint;
							ofVec2f projectorImagePoint;
							jsonCorrespondence["worldPoint"] >> worldPoint;
							jsonCorrespondence["projectorImagePoint"] >> projectorImagePoint;
							capture.worldPoints.push_back(worldPoint);
							capture.projectorImagePoints.push_back(projectorImagePoint);
						}
						this->captures.push_back(move(capture));
					}
				}

				//----------
				vector<int> ProjectorFromGraycode::getSelection() const {
					auto selectionString = this->parameters.selection.get();
					auto selectionItemStrings = ofSplitString(selectionString, ",", true, true);
					
					vector<int> selectionInts;
					for (auto & selectionStringItem : selectionItemStrings) {
						if (selectionStringItem.empty()) {
							continue;
						}
						auto value = ofToInt(selectionStringItem);
						if (value >= 0 && value < this->captures.size()) {
							selectionInts.push_back(value);
						}
					}

					if (selectionInts.empty()) {
						for (int i = 0; i < this->captures.size(); i++) {
							selectionInts.push_back(i);
						}
					}

					return selectionInts;
				}

				//----------
				void ProjectorFromGraycode::deleteSelection() {
					int index = 0;
					auto selection = this->getSelection();
					for (auto it = this->captures.begin(); it != this->captures.end(); index++) {
						if (find(selection.begin(), selection.end(), index) != selection.end()) {
							it = this->captures.erase(it);
						}
						else {
							it++;
						}
					}
					this->parameters.selection = "";
				}

			}
		}
	}
}

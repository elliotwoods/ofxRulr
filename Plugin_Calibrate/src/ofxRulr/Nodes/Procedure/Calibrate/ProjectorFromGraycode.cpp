#include "pch_Plugin_Calibrate.h"
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
#pragma mark Capture
				//----------
				ProjectorFromGraycode::Capture::Capture() {
					RULR_NODE_SERIALIZATION_LISTENERS;
				}

				//----------
				string ProjectorFromGraycode::Capture::getDisplayString() const {
					stringstream ss;
					ss << this->worldPoints.size() << " points";
					return ss.str();
				}

				//----------
				void ProjectorFromGraycode::Capture::serialize(Json::Value & json) {
					json["worldPoints"] << this->worldPoints;
					json["projectorImagePoints"] << this->projectorImagePoints;
				}

				//----------
				void ProjectorFromGraycode::Capture::deserialize(const Json::Value & json) {
					json["worldPoints"] >> this->worldPoints;
					json["projectorImagePoints"] >> this->projectorImagePoints;
				}

				//----------
				void ProjectorFromGraycode::Capture::drawWorld(shared_ptr<Item::Projector> projector)
				{
					ofMesh points;
					points.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);

					ofMesh zigzag;
					zigzag.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINE_STRIP);

					for (int i = 0; i < this->worldPoints.size(); i++) {
						const auto & worldPoint = this->worldPoints[i];
						if (i == 0) {
							ofPushStyle();
							{
								ofSetColor(this->color);
								ofDrawSphere(worldPoint, 0.05f);
							}
							ofPopStyle();
						}

						zigzag.addVertex(worldPoint);
						zigzag.addColor((float)i / (float)this->worldPoints.size());
					}
					zigzag.draw();

					Utils::Graphics::pushPointSize(5.0f);
					{
						points.drawVertices();
					}
					Utils::Graphics::popPointSize();

					//draw transform axes
					ofPushMatrix();
					{
						ofMultMatrix(this->transform);
						ofDrawAxis(0.1f);
					}
					ofPopMatrix();

					//draw lines to projector
					if (projector) {
						const auto projectorWidth = projector->getWidth();
						const auto projectorHeight = projector->getHeight();
						const auto projectorPosition = projector->getPosition();
						const auto projectorView = projector->getViewInWorldSpace();

						ofMesh projectorRays;
						projectorRays.setMode(ofPrimitiveMode::OF_PRIMITIVE_TRIANGLES);

						for (int i = 0; i < this->worldPoints.size(); i++) {
							const auto & worldPoint = this->worldPoints[i];
							auto color = ofFloatColor(
								this->projectorImagePoints[i].x / projector->getWidth(),
								this->projectorImagePoints[i].y / projector->getHeight(),
								0
							);
							points.addVertex(worldPoint);
							points.addColor(color);

							auto distance = worldPoint.distance(projectorPosition);
							auto projectorRay = projectorView.castPixel(this->projectorImagePoints[i]);
							projectorRays.addVertex(projectorRay.s);
							projectorRays.addVertex(projectorRay.s + projectorRay.t * distance / projectorRay.t.length());
							projectorRays.addVertex(worldPoint);

							projectorRays.addColor(ofFloatColor(0.5, 0.5, 0.5, 0.0f));
							color.a = 0.5f;
							projectorRays.addColor(color);
							color.a = 1.0f;
							projectorRays.addColor(color);
						}

						ofPushStyle();
						{
							ofEnableAlphaBlending();
							projectorRays.drawWireframe();
							ofDisableAlphaBlending();
						}
						ofPopStyle();
					}
					else {
						for (int i = 0; i < this->worldPoints.size(); i++) {
							const auto & worldPoint = this->worldPoints[i];
							points.addVertex(worldPoint);
						}
					}
				}

				//----------
				void ProjectorFromGraycode::Capture::drawOnCameraImage() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(ofxCv::toCv(this->cameraImagePoints), false);
					}
					ofPopStyle();
				}

				//----------
				void ProjectorFromGraycode::Capture::drawOnProjectorImage() {
					ofPushStyle();
					{
						ofSetColor(this->color);
						ofxCv::drawCorners(ofxCv::toCv(this->projectorImagePoints), false);
					}
					ofPopStyle();
				}

#pragma mark ProjectorFromGraycode
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
					RULR_NODE_UPDATE_LISTENER;
					RULR_NODE_DRAW_WORLD_LISTENER;
					RULR_NODE_INSPECTOR_LISTENER;
					RULR_NODE_SERIALIZATION_LISTENERS;

					this->addInput<Item::Projector>();
					this->addInput<Item::Camera>();
					this->addInput<Item::AbstractBoard>();
					this->addInput<Scan::Graycode>();

					{
						auto panel = ofxCvGui::Panels::Groups::makeStrip();
						
						{
							auto cameraPanel = ofxCvGui::Panels::makeTexture(this->preview.projectorInCamera, "Camera");
							cameraPanel->onDrawImage += [this](ofxCvGui::DrawImageArguments &) {
								auto captures = this->captures.getSelection();
								for (auto capture : captures) {
									capture->drawOnCameraImage();
								}
							};
							panel->add(cameraPanel);
						}
						{
							auto projectorPanel = ofxCvGui::Panels::makeTexture(this->preview.cameraInProjector, "Projector");
							projectorPanel->onDrawImage += [this](ofxCvGui::DrawImageArguments &) {
								auto captures = this->captures.getSelection();
								for (auto capture : captures) {
									capture->drawOnProjectorImage();
								}
							};
							panel->add(projectorPanel);
						}
						this->panel = panel;
					}
				}

				//----------
				void ProjectorFromGraycode::update() {
					{
						auto projector = this->getInput<Item::Projector>();
						if (projector) {
							auto width = this->preview.cameraInProjector.getWidth();
							auto height = this->preview.cameraInProjector.getHeight();
							if (width != projector->getWidth()
								|| height != projector->getHeight()) {
								this->preview.cameraInProjector.allocate(projector->getWidth()
									, projector->getHeight()
									, GL_LUMINANCE);
							}
						}
					}
					{
						auto camera = this->getInput<Item::Camera>();
						if (camera) {
							auto width = this->preview.projectorInCamera.getWidth();
							auto height = this->preview.projectorInCamera.getHeight();
							if (width != camera->getWidth()
								|| height != camera->getHeight()) {
								this->preview.projectorInCamera.allocate(camera->getWidth()
									, camera->getHeight()
									, GL_LUMINANCE);
							}
						}
					}
				}

				//----------
				void ProjectorFromGraycode::drawWorldStage() {
					auto projectorNode = this->getInput<Item::Projector>();

					float projectorWidth = 0, projectorHeight = 0;
					if (projectorNode) {
						projectorWidth = projectorNode->getWidth();
						projectorHeight = projectorNode->getHeight();
					}

					auto captures = this->captures.getSelection();
					for (auto & capture : captures) {
						capture->drawWorld(projectorNode);
					}
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
						Utils::ScopedProcess scopedProcess("Graycode scan", false);
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

						if (!boardNode->findBoard(medianCopyMat
							, pointsInCameraImage
							, boardObjectPoints
							, this->parameters.capture.findBoardMode
							, cameraNode->getCameraMatrix()
							, cameraNode->getDistortionCoefficients())) {
							throw(ofxRulr::Exception("Board not found in camera image"));
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
								, false);
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
							boardPointsInWorldSpace.push_back(toOf(boardObjectPoint) * (boardTransform * cameraNode->getTransform()));
						}
					}

					//build a look up mesh for projector coords within camera image
					ofMesh projectorInCameraMesh;
					{
						Utils::ScopedProcess scopedProcessBuildMesh("Build delauney mesh", false);
						auto & projectorInCameraPreview = graycodeNode->getDecoder().getProjectorInCamera().getPixels();
						auto boardBoundsInCamera = cv::boundingRect(Mat(pointsInCameraImage));

						//erode away the active (to reduce noise)
						auto activeEroded = dataSet.getActive();
						auto activeErodedMat = toCv(activeEroded);
						{
							ofVec2f boardSizeInCamera(boardBoundsInCamera.width, boardBoundsInCamera.height);
							float erosionSize = boardSizeInCamera.length() * this->parameters.capture.erosion;
							cv::erode(activeErodedMat, activeErodedMat, cv::Mat(), cv::Point(-1, -1), (int)erosionSize);
						}

						//make vertices and tex coords
						Delaunay::Point tempP;
						vector<Delaunay::Point> delauneyPoints;
						auto activeErodedPixel = activeEroded.getData();
						auto maskInCameraSpace = ofxCv::toOf(boardBoundsInCamera);
						for (auto & pixel : dataSet) {
							if (!*activeErodedPixel++) {
								continue;
							}

							auto projectorXY = pixel.getProjectorXY();
							auto cameraXY = pixel.getCameraXY();
							if (!maskInCameraSpace.inside(cameraXY)) {
								continue;
							}

							delauneyPoints.emplace_back(cameraXY.x, cameraXY.y);
							projectorInCameraMesh.addVertex(cameraXY);
							projectorInCameraMesh.addColor(ofFloatColor(projectorXY.x, projectorXY.y, 0));
						}

						//check delauney point count
						if (delauneyPoints.size() > this->parameters.capture.maximumDelauneyPoints) {
							stringstream message;
							message << "Too many points found in scan (" << delauneyPoints.size() << " found. Maximum is " << this->parameters.capture.maximumDelauneyPoints.get() << ")";
							throw(ofxRulr::Exception(message.str()));
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
					{
						auto capture = make_shared<Capture>();
						auto worldPoint = boardPointsInWorldSpace.begin();

						for (auto & cameraPoint : pointsInCameraImage) {
							auto projectorPoint = getColorSubpix<cv::Vec2f>(toCv(projectorInCameraPixels), cameraPoint);

							if (projectorPoint[0] == 0 && projectorPoint[1] == 0) {
								//skip projector and world points if we can't find the proj coord for this corner
								worldPoint++;
								continue;
							}

							capture->cameraImagePoints.push_back(toOf(cameraPoint));
							capture->projectorImagePoints.push_back(toOf((cv::Point2f) projectorPoint));
							capture->worldPoints.push_back(*worldPoint++);
						}
						capture->transform = boardTransform;
						this->captures.add(capture);
					}

					//make previews
					{
						this->preview.projectorInCamera.loadData(dataSet.getMedian());
						this->preview.cameraInProjector.loadData(dataSet.getMedianInverse());
					}
					scopedProcess.end();
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

					auto selection = this->captures.getSelection();
					for (auto capture : selection) {
						worldPoints.insert(worldPoints.end()
							, capture->worldPoints.begin()
							, capture->worldPoints.end());
						projectorImagePoints.insert(projectorImagePoints.end()
							, capture->projectorImagePoints.begin()
							, capture->projectorImagePoints.end());
					}
					cv::Mat cameraMatrix, rotation, translation;
					this->error = ofxCv::calibrateProjector(cameraMatrix
						, rotation
						, translation
						, worldPoints
						, projectorImagePoints
						, this->getInput<Item::Projector>()->getWidth()
						, this->getInput<Item::Projector>()->getHeight()
						, false
						, 0.5f
						, 1.4f);

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
						RULR_CATCH_ALL_TO_ALERT;
					}, ' ');
					this->captures.populateWidgets(inspector);
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
					this->captures.serialize(json["captures"]);
				}

				//----------
				void ProjectorFromGraycode::deserialize(const Json::Value & json) {
					Utils::Serializable::deserialize(json, this->parameters);
					this->captures.deserialize(json["captures"]);
				}
			}
		}
	}
}

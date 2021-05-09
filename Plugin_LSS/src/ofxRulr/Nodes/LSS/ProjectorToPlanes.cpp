#include "pch_Plugin_LSS.h"

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
			//----------
			ProjectorToPlanes::ProjectorToPlanes() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string ProjectorToPlanes::getTypeName() const {
				return "LSS::ProjectorToPlanes";
			}

			//----------
			void ProjectorToPlanes::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;

				this->addInput<Item::Camera>();
				this->addInput<Item::Projector>();
				this->addInput<Procedure::Scan::Graycode>();
				this->addInput<Planes>();

				this->manageParameters(this->parameters);

				{
					auto strip = ofxCvGui::Panels::Groups::makeStrip();
					
					{
						auto panel = ofxCvGui::Panels::makeImage(this->preview.edges, "Edges");
						strip->add(panel);
					}

					{
						auto panel = ofxCvGui::Panels::makeImage(this->preview.pixelsOnPlane, "Pixels on plane");
						strip->add(panel);
					}

					this->panel = strip;
				}
				 
			}

			//----------
			ofxCvGui::PanelPtr ProjectorToPlanes::getPanel() {
				return this->panel;
			}

			//----------
			void ProjectorToPlanes::drawWorldStage() {
				this->preview.projectorPixelsMesh.drawVertices();
			}

			//----------
			void ProjectorToPlanes::populateInspector(ofxCvGui::InspectArguments & args) {
				auto inspector = args.inspector;

				inspector->addButton("Calibrate", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Calibrate");
						this->calibrate();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, OF_KEY_RETURN)->setHeight(100.0f);

				inspector->addButton("Select projector pixels", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Select projector pixels");
						this->selectProjectorPixels();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Projector pixel positions", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Projector pixel positions");
						this->projectorPixelPositions();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addLiveValue<size_t>("Found projector pixels", [this]() {
					return this->data.projectorPixels.size();
				});

				inspector->addButton("Solve projector", [this]() {
					try {
						Utils::ScopedProcess scopedProcess("Solve projector");
						this->solveProjector();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addLiveValue<float>("Reprojection error [px]", [this]() {
					return this->data.reprojectionError;
				});
			}

			//----------
			void ProjectorToPlanes::serialize(nlohmann::json & json) {

			}

			//----------
			void ProjectorToPlanes::deserialize(const nlohmann::json & json) {

			}

			//----------
			void ProjectorToPlanes::calibrate() {
				this->selectProjectorPixels();
				this->projectorPixelPositions();
				this->solveProjector();
			}

			//----------
			void ProjectorToPlanes::selectProjectorPixels() {
				Utils::ScopedProcess scopedProcess("Select projector pixels");

				this->throwIfMissingAConnection<Procedure::Scan::Graycode>();
				auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();

				if (!graycodeNode->hasData()) {
					throw(ofxRulr::Exception("Graycode scan data not available"));
				}

				const auto & dataSet = graycodeNode->getDataSet();

				ofPixels medianPixels = dataSet.getMedian();
				auto median = ofxCv::toCv(medianPixels);

				//find edges
				cv::Canny(median
					, this->data.edges
					, this->parameters.edges.minimumThreshold
					, this->parameters.edges.minimumThreshold * this->parameters.edges.ratio
					, this->parameters.edges.apertureSize);

				//dilations
				{
					cv::Mat element = getStructuringElement(cv::MORPH_RECT,
						cv::Size(3, 3));

					for (int i = 0; i < this->parameters.edges.dilations.get(); i++) {
						cv::dilate(this->data.edges
							, this->data.edges
							, element);
					}
				}

				//mask
				{
					ofPixels activePixels = dataSet.getActive();
					auto active = ofxCv::toCv(activePixels);

					cv::Mat mask;
					cv::bitwise_not(this->data.pixelsOnPlane, mask);
					cv::threshold(this->data.edges
						, mask
						, this->parameters.edges.threshold
						, 255
						, cv::THRESH_BINARY_INV);
					cv::bitwise_and(mask
						, active
						, this->data.pixelsOnPlane);
				}

				//preview
				{
					ofxCv::copy(this->data.edges, this->preview.edges);
					ofxCv::copy(this->data.pixelsOnPlane, this->preview.pixelsOnPlane);

					this->preview.edges.update();
					this->preview.pixelsOnPlane.update();
				}

				scopedProcess.end();
			}

			//----------
			void ProjectorToPlanes::projectorPixelPositions() {
				Utils::ScopedProcess scopedProcess("Find projector pixel positions");

				this->throwIfMissingAConnection<Planes>();
				this->throwIfMissingAConnection<Item::Camera>();
				this->throwIfMissingAConnection<Procedure::Scan::Graycode>();

				auto planes = this->getInput<Planes>()->getPlanes();

				auto cameraNode = this->getInput<Item::Camera>();
				auto cameraView = cameraNode->getViewInWorldSpace();
				auto cameraPosition = cameraNode->getPosition();
				auto cameraLookDirection = cameraView.castCoordinate(glm::vec2(0, 0)).t;
				
				auto cameraMatrix = cameraNode->getCameraMatrix();
				auto distortionCoefficients = cameraNode->getDistortionCoefficients();

				auto & pixelsOnPlane = this->data.pixelsOnPlane.data;

				auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();
				if (!graycodeNode->hasData()) {
					throw(ofxRulr::Exception("No scan data"));
				}
				auto & dataSet = graycodeNode->getDataSet();

				float projectorWidth = dataSet.getPayloadWidth();
				float projectorHeight = dataSet.getPayloadHeight();

				// clear the result
				this->data.projectorPixels.clear();

				// iterate over pixels
				for (const auto & pixel : dataSet) {
					// threshold test
					if (!pixel.active) {
						continue;
					}

					// on plane test
					if (pixelsOnPlane[pixel.camera] == 0) {
						continue;
					}

					// get its ray
					glm::vec2 cameraXY = pixel.getCameraXY();
					if (this->parameters.projectPixels.customUndistort) {
						cameraXY = ofxCv::toOf(ofxCv::undistortImagePoints(vector<cv::Point2f>(1, ofxCv::toCv(cameraXY))
							, cameraMatrix
							, distortionCoefficients).front());
					}
					auto pixelRay = cameraView.castPixel(cameraXY, !this->parameters.projectPixels.customUndistort);

					// find the closest plane intersection to the camera
					auto closestDistance = std::numeric_limits<float>::max();
					glm::vec3 closestIntersectionPosition;

					shared_ptr<Planes::Plane> closestPlane;

					for (const auto & plane : planes) {
						glm::vec3 intersectionPosition;
						plane->getPlane().intersect(pixelRay, intersectionPosition);

						// check it's in front of camera
						if (glm::dot(intersectionPosition - cameraPosition, cameraLookDirection) < 0) {
							continue;
						}

						auto distance = glm::distance2(cameraPosition, intersectionPosition);
						if (distance < closestDistance) {
							closestIntersectionPosition = intersectionPosition;
							closestDistance = distance;
							closestPlane = plane;
						}
					}

					// skip masked planes
					if (closestPlane->parameters.mask) {
						continue;
					}

					// add it to the dataset
					if (closestIntersectionPosition != glm::vec3()) {
						ProjectorPixel projectorPixel;
						projectorPixel.world = closestIntersectionPosition;
						projectorPixel.projection = pixel.getProjectorXY();
						this->data.projectorPixels.emplace_back(move(projectorPixel));
					}
				}

				// build preview
				{
					ofMesh previewMesh;
					for (const auto & projectorPixel : this->data.projectorPixels) {
						previewMesh.addVertex(projectorPixel.world);
						previewMesh.addColor(ofFloatColor(
							projectorPixel.projection.x / projectorWidth,
							projectorPixel.projection.y / projectorHeight,
							0.0f,
							1.0f)
						);
					}
					previewMesh.setMode(ofPrimitiveMode::OF_PRIMITIVE_POINTS);
					swap(this->preview.projectorPixelsMesh, previewMesh);
				}

				scopedProcess.end();
			}

			//---------
			// From PhotoScan::CalibrateProjector
			void ProjectorToPlanes::solveProjector() {
				Utils::ScopedProcess scopedProcess("Solve projector");

				this->throwIfMissingAConnection<Item::Projector>();
				this->throwIfMissingAConnection<Procedure::Scan::Graycode>();

				auto projectorNode = this->getInput<Item::Projector>();
				auto graycodeNode = this->getInput<Procedure::Scan::Graycode>();
				
				if (this->data.projectorPixels.size() < 10) {
					throw(ofxRulr::Exception("Need to find at least 10 projector pixels before solving"));
				}

				// set projector width, height
				auto projectorWidth = graycodeNode->getDataSet().getPayloadWidth();
				auto projectorHeight = graycodeNode->getDataSet().getPayloadHeight();
				projectorNode->setWidth(projectorWidth);
				projectorNode->setHeight(projectorHeight);

				vector<cv::Point2f> projectorImagePoints;
				vector<cv::Point3f> worldPoints;
				for (const auto & projectorPixel : this->data.projectorPixels) {
					projectorImagePoints.push_back(ofxCv::toCv(projectorPixel.projection));
					worldPoints.push_back(ofxCv::toCv(projectorPixel.world));
				}

				//initialise the projector fov
				auto cameraMatrix = projectorNode->getCameraMatrix();
				auto distortionCoefficients = projectorNode->getDistortionCoefficients();
				auto initParams = [&]() {
					auto initialThrowRatio = this->parameters.calibrate.initial.throwRatio;
					auto initialLensOffset = this->parameters.calibrate.initial.lensOffset;

					cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
					cameraMatrix.at<double>(0, 0) = projectorWidth * initialThrowRatio;
					cameraMatrix.at<double>(1, 1) = projectorWidth * initialThrowRatio;
					cameraMatrix.at<double>(0, 2) = projectorWidth / 2.0f;
					cameraMatrix.at<double>(1, 2) = projectorHeight * (0.50f - initialLensOffset / 2.0f);
					distortionCoefficients = cv::Mat::zeros(5, 1, CV_64F);
				};
				initParams();

				vector<cv::Mat> rotationVectors, translations;

				// Decimate the data
				if (this->parameters.calibrate.decimateData > 1) {
					cout << "Decimating data by " << this->parameters.calibrate.decimateData.get() << endl;

					auto projectorImagePointsIt = projectorImagePoints.begin();
					auto worldPointsIt = worldPoints.begin();

					auto decimator = max(this->parameters.calibrate.decimateData.get(), 1);

					int index = 0;

					vector<cv::Point3f> decimatedWorldPoints;
					vector<cv::Point2f> decimatedProjectorImagePoints;

					while(projectorImagePointsIt != projectorImagePoints.end()) {
						if (index == 0) {
							decimatedWorldPoints.push_back(*worldPointsIt);
							decimatedProjectorImagePoints.push_back(*projectorImagePointsIt);
						}

						projectorImagePointsIt++;
						worldPointsIt++;
						index++;
						index %= decimator;
					}

					swap(worldPoints, decimatedWorldPoints);
					swap(projectorImagePoints, decimatedProjectorImagePoints);
				}

				auto calib = [&]() {
					cout << "Calibrating projector with " << worldPoints.size() << " points" << endl;

					this->data.reprojectionError = cv::calibrateCamera(vector<vector<cv::Point3f>>(1, worldPoints)
						, vector<vector<cv::Point2f>>(1, projectorImagePoints)
						, projectorNode->getSize()
						, cameraMatrix
						, distortionCoefficients
						, rotationVectors
						, translations
						, cv::CALIB_FIX_K1
						| cv::CALIB_FIX_K2
						| cv::CALIB_FIX_K3
						| cv::CALIB_FIX_K4
						| cv::CALIB_FIX_K5
						| cv::CALIB_FIX_K6
						| cv::CALIB_ZERO_TANGENT_DIST
						| cv::CALIB_USE_INTRINSIC_GUESS
						| cv::CALIB_FIX_ASPECT_RATIO);
					cout << "Reprojection error " << this->data.reprojectionError << endl;
				};
				calib();

				//check reprojections
				if (this->parameters.calibrate.filterOutliers) {
					cout << "Filtering outliers" << endl;

					vector<cv::Point2f> imagePointsReprojected;
					cv::projectPoints(worldPoints
						, rotationVectors.front()
						, translations.front()
						, cameraMatrix
						, distortionCoefficients
						, imagePointsReprojected);

					set<int> outlierIndicies;
					for (int i = 0; i < imagePointsReprojected.size(); i++) {
						auto delta = ofxCv::toOf(imagePointsReprojected[i]) - ofxCv::toOf(projectorImagePoints[i]);
						auto reprojectionError = delta.length();

						if (reprojectionError > this->parameters.calibrate.maximumReprojectionError) {
							outlierIndicies.insert(i);
						}
					}

					cout << outlierIndicies.size() << " outliers found." << endl;

					//trim world points
					{
						auto index = 0;
						for (auto it = worldPoints.begin(); it != worldPoints.end(); ) {
							if (outlierIndicies.find(index++) != outlierIndicies.end()) {
								it = worldPoints.erase(it);
							}
							else {
								it++;
							}
						}
					}

					//trim image points
					{
						auto index = 0;
						for (auto it = projectorImagePoints.begin(); it != projectorImagePoints.end(); ) {
							if (outlierIndicies.find(index++) != outlierIndicies.end()) {
								it = projectorImagePoints.erase(it);
							}
							else {
								it++;
							}
						}
					}

					//calib again with trimmed points
					initParams();
					calib();
				}

				projectorNode->setIntrinsics(cameraMatrix
					, distortionCoefficients);
				projectorNode->setExtrinsics(rotationVectors.front()
					, translations.front()
					, true);

				scopedProcess.end();
			}
		}
	}
}
#include "pch_Plugin_LSS.h"

using json = nlohmann::json;

namespace ofxRulr {
	namespace Nodes {
		namespace LSS {
#pragma mark Scan
			//----------
			Projector::Scan::Scan() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			string Projector::Scan::getDisplayString() const {
				stringstream message;
				message << this->projectorPixels.size() << " projector pixels" << endl;
				message << "Camera position : " << this->cameraPosition.get();
				return message.str();
			}

			//----------
			string Projector::Scan::getTypeName() const {
				return "Scan";
			}

			//----------
			void Projector::Scan::deserialize(const Json::Value & json) {
				if (json.isMember("filename")) {
					auto filename = json["filename"].asString();
					ofxMessagePack::Unpacker unpacker;
					unpacker.load(filename);
					if (!unpacker) {
						throw(ofxRulr::Exception("Couldn't load scan "  + filename));
					}
					this->projectorPixels.clear();
					unpacker >> this->projectorPixels;
				}

				Utils::Serializable::deserialize(json, this->cameraPosition);

				this->previewDirty = true;
			}

			//----------
			void Projector::Scan::serialize(Json::Value & json) {
				ofxMessagePack::Packer packer;
				packer << this->projectorPixels;
				auto filename = this->getFilename();
				packer.save(filename);
				json["filename"] = filename;

				Utils::Serializable::serialize(json, this->cameraPosition);
			}

			//----------
			string Projector::Scan::getFilename() const {
				string filename = "LSS-Projector-Scan";

				{
					time_t time = chrono::system_clock::to_time_t(this->timestamp.get());
					char buffer[100];
					strftime(buffer, 100, "%Y.%m.%d %H.%M.%S", localtime(&time));
					filename += " " + string(buffer);
				}

				filename += ".bin";

				return filename;
			}

			//----------
			void Projector::Scan::drawWorld() {
				if (this->previewDirty) {
					this->rebuildPreview();
				}
				ofPushStyle();
				{
					ofEnableAlphaBlending();
					auto color = this->color.get();
					color.a = 10;
					ofSetColor(color);
					this->preview.draw();
				}
				ofPopStyle();
			}

			//----------
			void Projector::Scan::rebuildPreview() {
				ofMesh mesh;
				mesh.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);
				for (const auto & projectorPixel : this->projectorPixels) {
					const auto & ray = projectorPixel.second.cameraPixelRay;
					mesh.addVertex(ray.getStart());
					mesh.addVertex(ray.getEnd());
				}
				swap(mesh, this->preview);
				this->previewDirty = false;
			}

#pragma mark Projector
			//----------
			Projector::Projector() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string Projector::getTypeName() const {
				return "LSS::Projector";
			}

			//----------
			void Projector::init() {
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_DRAW_WORLD_LISTENER;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->addInput<Item::Projector>();
				this->addInput<System::VideoOutput>();
				
				{
					auto worldInputPin = this->addInput<World>();
					worldInputPin->onNewConnection += [this](shared_ptr<World> world) {
						world->add(static_pointer_cast<Projector>(this->shared_from_this()));
					};
					worldInputPin->onDeleteConnection += [this](shared_ptr<World> world) {
						if (world) {
							world->remove(static_pointer_cast<Projector>(this->shared_from_this()));
						}
					};
				}

				{
					auto stripPanel = ofxCvGui::Panels::Groups::makeStrip();
					stripPanel->setDirection(ofxCvGui::Panels::Groups::Strip::Direction::Vertical);
					{
						auto panel = ofxCvGui::Panels::makeWidgets();
						this->scans.populateWidgets(panel);
						stripPanel->add(panel);
					}
					{
						auto panel = ofxCvGui::Panels::makeImage(this->projectorSpacePreview);
						panel->onDrawImage += [this](ofxCvGui::DrawImageArguments & args) {
							ofPushMatrix();
							{
								ofScale(this->projectorSpacePreview.getWidth(), this->projectorSpacePreview.getHeight());
								ofTranslate(0.5, 0.5);
								ofScale(0.5f, -0.5f);

								if (this->parameters.draw.linesOnProjectorPreview) {
									ofPushStyle();
									{
										for (const auto & line : this->lines) {
											ofSetColor(line.color);
											ofDrawLine(line.startProjector, line.endProjector);
											if (!line.vertices.empty()) {
												ofDrawBitmapString(ofToString(line.vertices.size()), line.startProjector);
											}
										}
									}
									ofPopStyle();
								}
							}
							ofPopMatrix();
						};
						stripPanel->add(panel);
					}
					this->panel = stripPanel;
				}
				
				this->manageParameters(this->parameters);
			}

			//----------
			void Projector::update() {
				if (this->previewsDirty) {
					this->rebuildPreviews();
				}

				//allocate preview
				{
					auto projector = this->getInput<Item::Projector>();
					if (projector){
						if (projector->getWidth() != this->projectorSpacePreview.getWidth()
							|| projector->getHeight() != this->projectorSpacePreview.getHeight()) {
							this->projectorSpacePreview.allocate(projector->getWidth(), projector->getHeight(), OF_IMAGE_COLOR_ALPHA);
							this->projectorSpacePreview.getPixels().setColor(ofColor(0, 255));
							this->projectorSpacePreview.update();
						}
					}
				}
			}

			//----------
			void Projector::drawWorldStage() {
				if (this->checkDrawOnWorldStage(this->parameters.draw.unclassifiedVertices)) {
					this->unclassifiedVerticesPreview.draw(GL_POINTS, 0, this->unclassifiedVertices.size());
				}

				if (this->checkDrawOnWorldStage(this->parameters.draw.lines)) {
					for (const auto & line : this->lines) {
						line.drawWorld();
					}
				}
				if (this->checkDrawOnWorldStage(this->parameters.draw.rays)) {
					auto scans = this->scans.getSelection();
					for (const auto & scan : scans) {
						scan->drawWorld();
					}
				}
			}

			//----------
			void Projector::deserialize(const Json::Value & json) {
				this->scans.deserialize(json);

				//unclassifiedVertices
				if (json.isMember("unclassifiedVertices")) {
					const auto & jsonUnclassifiedVertices = json["unclassifiedVertices"];
					string filename = jsonUnclassifiedVertices["filename"].asString();

					ofxMessagePack::Unpacker unpacker;
					unpacker.load(filename);
					unpacker >> this->unclassifiedVertices;
				}

				//lines
				if (json.isMember("lines")) {
					const auto & jsonLines = json["lines"];
					string filename = jsonLines["filename"].asString();

					ofxMessagePack::Unpacker unpacker;
					unpacker.load(filename);
					unpacker >> this->lines;
				}
			}

			//----------
			void Projector::serialize(Json::Value & json) {
				this->scans.serialize(json);

				//unclassifiedVertices
				if (!this->unclassifiedVertices.empty()) {
					auto & jsonUnclassifiedVertices = json["unclassifiedVertices"];
					ofxMessagePack::Packer packer;
					packer << this->unclassifiedVertices;
					auto filename = this->getDefaultFilename() + "-unclassifiedVertices.bin";
					packer.save(filename);
					jsonUnclassifiedVertices["filename"] = filename;
					jsonUnclassifiedVertices["count"] = (int) this->unclassifiedVertices.size();
				}

				//lines
				if (!this->lines.empty()) {
					auto & jsonLines = json["lines"];
					ofxMessagePack::Packer packer;
					packer << this->lines;
					auto filename = this->getDefaultFilename() + "-lines.bin";
					packer.save(filename);
					jsonLines["filename"] = filename;
					jsonLines["count"] = (int) this->lines.size();
				}
			}

			//----------
			void Projector::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;
				inspector->addButton("Triangulate", [this]() {
					Utils::ScopedProcess scopedProcess("Triangulate " + this->getName());
					try {
						this->triangulate(this->parameters.maximumResidual);
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, OF_KEY_RETURN)->setHeight(100.0f);
				inspector->addLiveValue<size_t>("Unclassified vertices", [this]() {
					return this->unclassifiedVertices.size();
				});
				inspector->addButton("Auto mapping", [this]() {
					this->autoMapping(this->parameters.lineSearch);
				});
				inspector->addLiveValue<size_t>("Line count", [this]() {
					return this->lines.size();
				});
				inspector->addButton("Clear lines", [this]() {
					this->lines.clear();
				});
				inspector->addButton("Load mapping..,", [this]() {
					try {
						auto result = ofSystemLoadDialog("Select mapping json");
						if (result.bSuccess) {
							this->loadMapping(result.filePath);
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
				});

				inspector->addButton("Dip lines in data..,", [this]() {
					try {
						this->dipLinesInData();
					}
					RULR_CATCH_ALL_TO_ALERT;
				}, 'd')->setHeight(100.0f);
				inspector->addButton("Trim lines except one...", [this]() {
					try {
						auto lineIndexString = ofSystemTextBoxDialog("Line index");
						if (!lineIndexString.empty()) {
							auto lineIndex = ofToInt(lineIndexString);
							this->trimLinesExceptOne(lineIndex);
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
				});
			}

			//----------
			void Projector::addScan(shared_ptr<Scan> scan) {
				this->throwIfMissingAConnection<Item::Projector>();
				this->scans.add(scan);
			}

			//----------
			class VertexFindModel : public ofxNonLinearFit::Models::Base<Projector::ProjectorPixelFind, VertexFindModel> {
			public:
				unsigned int getParameterCount() const override {
					return 3;
				}

				void getResidual(Projector::ProjectorPixelFind find, double & residual, double * gradient) const override {
					residual = find.cameraPixelRay.distanceTo(this->point);
				}

				virtual void evaluate(Projector::ProjectorPixelFind &) const override {
					RULR_WARNING << "We shouldn't do this";
				}

				virtual void cacheModel() override {
					this->point = ofVec3f(this->parameters[0], this->parameters[1], this->parameters[2]);
				}

				ofVec3f point;
			};

			//----------
			void Projector::triangulate(float maximumResidual) {
				this->throwIfMissingAConnection<Item::Projector>();
				auto projector = this->getInput<Item::Projector>();

				const auto projectorWidth = (uint32_t)projector->getWidth();
				const auto projectorHeight = (uint32_t)projector->getHeight();

				map<uint32_t, vector<ProjectorPixelFind>> projectorPixelFindsSets;
				{
					//gather all rays per pixel
					auto scans = this->scans.getSelection();
					for (auto scan : scans) {
						for (const auto & projectorPixel : scan->projectorPixels) {
							projectorPixelFindsSets[projectorPixel.first].push_back(projectorPixel.second);
						}
					}
				}

				vector<Vertex> unclassifiedVertices;
				{
					Utils::ScopedProcess scopedProcessFitPoints("Fit points", false, 100);
					auto pointsPerPercent = projectorPixelFindsSets.size() / 100;
					int count = 0;

					for (const auto & projectorPixelFindSet : projectorPixelFindsSets) {
						if (count++ > pointsPerPercent) {
							Utils::ScopedProcess scopedProcessDummy("Found " + ofToString(unclassifiedVertices.size()) + "points", false);
							count = 0;
						}

						if (projectorPixelFindSet.second.size() < 2) {
							continue;
						}
						
						//perform LSF to find the pixel
						{
							ofxNonLinearFit::Fit<VertexFindModel> fit;
							VertexFindModel model;
							
							double residual;
							fit.optimise(model, &projectorPixelFindSet.second, & residual);
							if (residual <= maximumResidual) {
								Vertex vertex;
								vertex.world = model.point;
								vertex.projector = projectorPixelFindSet.first;

								auto projectorPixelCoordinatesX = vertex.projector % projectorWidth;
								auto projectorPixelCoordinatesY = vertex.projector / projectorWidth;

								vertex.projectorNormalizedXY.x = ofMap(projectorPixelCoordinatesX, 0, projectorWidth - 1, -1, +1);
								vertex.projectorNormalizedXY.x = ofMap(projectorPixelCoordinatesY, 0, projectorHeight - 1, +1, -1);

								unclassifiedVertices.emplace_back(move(vertex));
							}
						}
					}
				}
				swap(this->unclassifiedVertices, unclassifiedVertices);
				this->previewsDirty = true;
			}

			//----------
			void Projector::autoMapping(const LineSearchParams & params) {
				//this function classifies vertices

				this->previewsDirty = true;

				struct VertexStatus {
					bool classified = false;
				};

				class LineModel : public ofxNonLinearFit::Models::Base<Vertex, LineModel> {
				public:
					LineModel() {
						this->ray.infinite = true;
					}

					unsigned int getParameterCount() const override {
						return 4;
					}

					void getResidual(Vertex vertex, double & residual, double * gradient = 0) const override {
						residual = this->ray.distanceTo(vertex.world);
					}

					void evaluate(Vertex  &) const override {
						RULR_ERROR << "We shouldn't be here";
					}

					virtual void cacheModel() override {
						this->ray.s.x = this->parameters[0];
						this->ray.s.y = this->parameters[1];
						this->ray.s.z = 0.0f;

						auto theta = this->parameters[2];
						auto thi = this->parameters[3];
						this->ray.t.x = cos(theta) * cos(thi);
						this->ray.t.y = sin(theta) * cos(thi);
						this->ray.t.z = sin(thi);
					}

					ofxRay::Ray ray;
				};

				//setup some constants
				const auto availableVertices = this->unclassifiedVertices;
				const auto size = availableVertices.size();
				vector<VertexStatus> vertexStatuses(size);

				const auto headSize = params.headSize.get();
				const auto headSize2 = headSize * headSize;
				const auto trunkThickness = params.trunkThickness.get();
				const auto initialInclusionThreshold = params.trunkThickness.get();
				const auto minimumCount = params.minimumCount.get();

				auto getNeighborhoodIndices = [&](const ofVec3f & position) {
					//find local vertices
					set<size_t> neighborhoodVertexIndices;
					for (size_t iVertexOther = 0; iVertexOther < size; iVertexOther++) {
						if (!vertexStatuses[iVertexOther].classified) {
							if (availableVertices[iVertexOther].world.squareDistance(position) <= headSize2) {
								neighborhoodVertexIndices.insert(iVertexOther);
							}
						}
					}
					return neighborhoodVertexIndices;
				};

				auto getVerticesFromIndices = [&](const set<size_t> & indices) {
					vector<Vertex> result;
					for (const auto & index : indices) {
						result.push_back(availableVertices[index]);
					}
					return result;
				};

				auto fitRay = [&](const vector<Vertex> & vertices) {
					ofxNonLinearFit::Fit<LineModel> fit;
					LineModel lineModel;
					fit.optimise(lineModel, &vertices);

					return lineModel.ray;
				};

				auto countInsideTrunk = [&](const vector<Vertex> & vertices, const ofxRay::Ray & ray) {
					size_t countInsideTrunk = 0;
					for (const auto & vertex : vertices) {
						if (ray.distanceTo(vertex.world) < trunkThickness) {
							countInsideTrunk++;
						}
					}
					return countInsideTrunk;
				};

				auto checkPopulationThreshold = [&](const ofxRay::Ray & ray, const vector<Vertex> & vertices) {
					const auto count = countInsideTrunk(vertices, ray);
					cout << count << ", ";
					return (float) count / (float)vertices.size() >= initialInclusionThreshold;
				};

				auto addToLineSetInsideTrunk = [&](const set<size_t> & vertexIndices, const ofxRay::Ray & ray, set<size_t> & lineSet) {
					for (const auto & vertexIndex : vertexIndices) {
						const auto & vertex = availableVertices[vertexIndex];
						if (ray.distanceTo(vertex.world) <= trunkThickness) {
							lineSet.insert(vertexIndex);
						}
					}
				};

				for (int iVertex = 0; iVertex < size; iVertex++) {
					auto & vertexStatus = vertexStatuses[iVertex];
					const auto & vertex = availableVertices[iVertex];

					//classified vertices should be ignored
					if (vertexStatus.classified) {
						continue;
					}

					set<size_t> lineSet;
					ofxRay::Ray ray;
					ray.s = vertex.world;

					//walk along the line
					for (float u = 0; true; u += headSize) {
						const auto searchPosition = ray.s + ray.t * u;
						const auto neighborhoodIndices = getNeighborhoodIndices(searchPosition);

						if (neighborhoodIndices.size() < minimumCount) {
							//not enough vertices to continue
							break; //break out of extension loop
						}

						auto extendedLineSet = lineSet;
						extendedLineSet.insert(neighborhoodIndices.begin(), neighborhoodIndices.end());
						const auto extendedVertexSet = getVerticesFromIndices(extendedLineSet);
						auto extendedLine = fitRay(extendedVertexSet);

						if (lineSet.empty()) {
							//we're starting the line trunk here
							if (!checkPopulationThreshold(extendedLine, extendedVertexSet)) {
								//no vertices
								break; //exit creating the line
							}
						}
						else {
							//we're extending an existing line

							//check extended line contains more points than before
							auto countInsideNewTrunk = countInsideTrunk(extendedVertexSet, extendedLine);
							if (countInsideNewTrunk <= lineSet.size()) {
								break; //end of line
							}
						}

						//classify matching vertices
						addToLineSetInsideTrunk(neighborhoodIndices, ray, lineSet);

						//update line
						auto trunkVertices = getVerticesFromIndices(lineSet);
						ray = fitRay(trunkVertices);
					}

					if (!lineSet.empty()) {
						//we have a new line
						for (const auto & index : lineSet) {
							vertexStatuses[index].classified = true;
						}
						const auto vertices = getVerticesFromIndices(lineSet);
						Line newLine;
						newLine.startWorld = ray.getStart();
						newLine.endWorld = ray.getEnd();
						newLine.vertices = vertices;
						this->lines.push_back(newLine);
					}
				}
			}

			//----------
			void Projector::loadMapping(const string & filename) {
				this->lines.clear();
				
				json jsonLines;
				{
					ofFile jsonFile;
					jsonFile.open(filename, ofFile::ReadOnly, false);
					jsonLines = json::parse(jsonFile.readToBuffer().getText());
				}
				for (const auto & jsonLine : jsonLines) {
					Line line;
					line.projectorIndex = jsonLine["ProjectorIndex"].get<int>();
					if (line.projectorIndex != this->parameters.projectorIndex.get()) {
						continue;
					}

					line.age = jsonLine["Age"].get<double>();
					line.lastEditBy = jsonLine["LastEditBy"].get<string>();
					line.lastUpdate = jsonLine["LastUpdate"].get<string>();
					line.startProjector.x = jsonLine["Start"]["x"].get<float>();
					line.startProjector.y = jsonLine["Start"]["y"].get<float>();
					line.endProjector.x = jsonLine["End"]["x"].get<float>();
					line.endProjector.y = jsonLine["End"]["y"].get<float>();
					line.projectorIndex = jsonLine["ProjectorIndex"].get<int>();
					line.lineIndex = jsonLine["LineIndex"].get<int>();

					this->lines.push_back(line);
				}

				this->previewsDirty = true;
			}

			//----------
			//from http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
			//v=start, w=end, p=point
			float minimum_distance(ofVec2f start, ofVec2f end, ofVec2f point) {
				// Return minimum distance between line segment vw and point p
				const float l2 = (start - end).lengthSquared();  // i.e. |w-v|^2 -  avoid a sqrt
				if (l2 == 0.0) return (point - start).lengthSquared();   // v == w case

																		 // Consider the line extending the segment, parameterized as v + t (w - v).
																		 // We find projection of point p onto the line.
																		 // It falls where t = [(p-v) . (w-v)] / |w-v|^2
				const float t = (point - start).dot(end - start) / l2;
				if (t < 0.0) return (point - start).length();       // Beyond the 'v' end of the segment
				else if (t > 1.0) return (point - end).length();  // Beyond the 'w' end of the segment
				const ofVec2f projection = start + t * (end - start);  // Projection falls on the segment
				return (point - projection).length();
			}

			//----------
			void Projector::dipLinesInData() {
				this->throwIfMissingAConnection<Item::Projector>();

				if (this->unclassifiedVertices.empty()) {
					throw(ofxRulr::Exception("No unclassified vertices to use for data dip"));
				}
				Utils::ScopedProcess scopedProcess("Dip lines in data", true, this->lines.size());
				
				auto projector = this->getInput<Item::Projector>();
				auto projectorWidth = projector->getWidth();
				auto projectorHeight = projector->getHeight();

				auto & previewPixels = this->projectorSpacePreview.getPixels();
				previewPixels.setColor(ofColor(0, 255));
				auto previewPixelsData = (ofColor*) previewPixels.getData();

				for (auto & line : this->lines) {
					Utils::ScopedProcess scopedProcessLine("Dip line by " + ofToString(line.lastEditBy), false);

					//pass through vertices
					for (auto it = this->unclassifiedVertices.begin(); it != this->unclassifiedVertices.end();) {
						const auto & vertex = *it;
						
						//transform to pixel coordinates
						ofVec2f start, end;
						{
							start.x = ofMap(line.startProjector.x, -1, +1, 0, projectorWidth - 1);
							start.y = ofMap(line.startProjector.y, +1, -1, 0, projectorHeight - 1);
							end.x = ofMap(line.endProjector.x, -1, +1, 0, projectorWidth - 1);
							end.y = ofMap(line.endProjector.y, +1, -1, 0, projectorHeight - 1);

							auto pixelIndex = (uint32_t) start.x + (uint32_t) start.y * (uint32_t) projectorHeight;
							previewPixelsData[pixelIndex].r = 255;
						}

						//get pixel coords of projector pixel
						ofVec2f projector;
						{
							projector.x = vertex.projector % (int) projectorWidth;
							projector.y = vertex.projector / (int) projectorWidth;
							previewPixelsData[vertex.projector].g = 255;
						}

						//check distance is within thickness
						auto distanceToLine = minimum_distance(start, end, projector);
						if (distanceToLine > this->parameters.dataDip.searchThickness) {
							it++;
							continue;
						}

						previewPixelsData[vertex.projector].b = 255;
						line.vertices.push_back(vertex);
						it = this->unclassifiedVertices.erase(it);
					}
				}
				this->projectorSpacePreview.update();
				this->previewsDirty = true;
				scopedProcess.end();
			}

			//----------
			void Projector::trimLinesExceptOne(int lineIndex) {
				for (const auto & line : this->lines) {
					if (line.lineIndex == lineIndex) {
						auto lineCopy = line;
						this->lines.clear();
						this->lines.push_back(lineCopy);
						break;
					}
				}
			}

			//----------
			ofxCvGui::PanelPtr Projector::getPanel() {
				return this->panel;
			}

			//----------
			std::vector<Projector::Line> & Projector::getLines() {
				return this->lines;
			}

			//----------
			void Projector::rebuildPreviews() {
				{
					ofVbo unclassifiedVerticesPreview;
					vector<ofVec3f> vertices;
					for (const auto & vertex : this->unclassifiedVertices) {
						vertices.push_back(vertex.world);
					}
					unclassifiedVerticesPreview.setVertexData(vertices.data(), vertices.size(), GL_STATIC_DRAW);
					swap(unclassifiedVerticesPreview, this->unclassifiedVerticesPreview);
				}

				{
					for (auto & line : this->lines) {
						ofVbo vbo;
						vector<ofVec3f> vertices;
						for (const auto & vertex : line.vertices) {
							vertices.push_back(vertex.world);
						}
						vbo.setVertexData(vertices.data(), vertices.size(), GL_STATIC_DRAW);
						swap(vbo, line.vbo);
					}
				}
				this->previewsDirty = false;
			}

#pragma mark Line
			//----------
			void Projector::Line::drawWorld() const {
				ofPushStyle();
				{
					ofSetColor(this->color);
					if (!this->vertices.empty()) {

						this->vbo.draw(GL_POINTS, 0, this->vertices.size());

					}
					if (this->startWorld != this->endWorld) {
						ofDrawLine(this->startWorld, this->endWorld);
					}
				}
				ofPopStyle();
			}
		}
	}
}
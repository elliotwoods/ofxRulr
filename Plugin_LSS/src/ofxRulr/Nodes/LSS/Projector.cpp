#include "pch_Plugin_LSS.h"

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
					auto panel = ofxCvGui::Panels::makeWidgets();
					this->scans.populateWidgets(panel);
					this->panel = panel;
				}

				this->manageParameters(this->parameters);
			}

			//----------
			void Projector::drawWorldStage() {
				if (this->checkDrawOnWorldStage(this->parameters.drawRays)) {
					auto scans = this->scans.getSelection();
					for (const auto & scan : scans) {
						scan->drawWorld();
					}
				}
				
				if (this->checkDrawOnWorldStage(this->parameters.drawVertices)) {
					this->triangulatedMesh.drawVertices();
				}
			}

			//----------
			void Projector::deserialize(const Json::Value & json) {
				this->scans.deserialize(json);
			}

			//----------
			void Projector::serialize(Json::Value & json) {
				this->scans.serialize(json);
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
				inspector->addLiveValue<size_t>("Triangulated points", [this]() {
					return this->triangulatedMesh.getNumVertices();
				});
			}

			//----------
			void Projector::addScan(shared_ptr<Scan> scan) {
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

				ofMesh vertices;
				{
					Utils::ScopedProcess scopedProcessFitPoints("Fit points", false, 100);
					auto pointsPerPercent = projectorPixelFindsSets.size() / 100;
					int count = 0;

					vertices.setMode(OF_PRIMITIVE_POINTS);
					for (const auto & projectorPixelFindSet : projectorPixelFindsSets) {
						if (count++ > pointsPerPercent) {
							Utils::ScopedProcess scopedProcessDummy("Found " + ofToString(vertices.getNumVertices()) + "points", false);
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
								vertices.addVertex(model.point);
							}
						}
					}
				}
				swap(this->triangulatedMesh, vertices);
			}

			//----------
			ofxCvGui::PanelPtr Projector::getPanel() {
				return this->panel;
			}
		}
	}
}
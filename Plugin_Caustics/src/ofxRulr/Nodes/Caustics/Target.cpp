#include "pch_Plugin_Caustics.h"
#include "Target.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Caustics {
#pragma mark Curve
			//----------
			void
				Target::Curve::serialize(nlohmann::json& json) const
			{
				{
					nlohmann::json jsonVertices;
					for (auto& vertex : this->vertices) {
						nlohmann::json jsonVertex;
						jsonVertex[0] = vertex[0];
						jsonVertex[1] = vertex[1];
						jsonVertices.push_back(jsonVertex);
					}
					json["vertices"] = jsonVertices;
				}
			}

			//----------
			void
				Target::Curve::deserialize(const nlohmann::json& json)
			{
				this->vertices.clear();

				if (json.contains("vertices")) {
					const auto& jsonVertices = json["vertices"];
					for (const auto& jsonVertex : jsonVertices) {
						if (jsonVertex.size() >= 2) {
							glm::vec3 vertex{
								(float)jsonVertex[0]
								, (float)jsonVertex[1]
								, 0.0f
							};
							this->vertices.push_back(vertex);
						}
					}
				}

				this->calc();
			}

			//----------
			void
				Target::Curve::clear()
			{
				this->vertices.clear();
				this->calc();
			}

			//----------
			void
				Target::Curve::addVertex(const glm::vec3& vertex)
			{
				this->vertices.push_back(vertex);
				this->calc();
			}

			//----------
			void
				Target::Curve::addVertices(const vector<glm::vec3>& vertices)
			{
				this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
				this->calc();
			}

			//----------
			float
				Target::Curve::getLength() const
			{
				return this->length;
			}

			//----------
			const vector<glm::vec3>&
				Target::Curve::getVertices() const
			{
				return this->vertices;
			}

			//----------
			vector<glm::vec3>&
				Target::Curve::getVerticesForWriting()
			{
				return this->vertices;
			}

			//----------
			void
				Target::Curve::resample(size_t size)
			{
				this->calc();

				// We cheat by using the preview ofPolyLine which has the interpolation method already
				this->vertices.clear();
				for (size_t i = 0; i < size; i++) {
					float ratio = (float)i / (float)(size - 1);
					this->vertices.push_back(this->preview.line.getPointAtPercent(ratio));
				}

				this->calc();
			}

			//----------
			void
				Target::Curve::draw(const DrawArguments& args) const
			{
				this->preview.line.draw();

				if (args.showInfo && !this->vertices.empty()) {
					stringstream notes;
					notes << "V : " << this->vertices.size() << std::endl;
					notes << "L : " << this->getLength();

					ofxCvGui::Utils::drawTextAnnotation(notes.str(), this->vertices[0]);
				}
			}

			//----------
			void
				Target::Curve::calc()
			{
				this->verticesByPosition.clear();
				this->preview.line.clear();
				this->length = 0.0f;

				if (this->vertices.size() < 2) {
					return;
				}

				// Calculate the total length
				{
					// start with first 2 vertices
					auto previousVertex = this->vertices.cbegin();
					auto vertex = previousVertex;

					// store first vertex
					this->verticesByPosition.emplace(0.0f, *previousVertex);

					// iterate through vertices
					vertex++;
					for (; vertex != this->vertices.end(); vertex++) {
						this->length += glm::distance(*previousVertex, *vertex);
						this->verticesByPosition.emplace(this->length, *vertex);
						previousVertex = vertex;
					}
				}

				// Prepare the preview
				{
					this->preview.line.addVertices(this->vertices);
				}
			}

#pragma mark Target
			//----------
			Target::Target()
			{
				RULR_NODE_INIT_LISTENER;
				this->setIconGlyph(u8"\uf140");
			}

			//----------
			string
				Target::getTypeName() const
			{
				return "Caustics::Target";
			}

			//----------
			void Target::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_UPDATE_LISTENER;

				RULR_RIGIDBODY_DRAW_OBJECT_LISTENER;

				this->manageParameters(this->parameters);
			}

			//----------
			void Target::update()
			{

			}

			//----------
			ofxCvGui::PanelPtr
				Target::getPanel()
			{
				auto panel = ofxCvGui::Panels::makeBlank();

				panel->onDraw += [this](ofxCvGui::DrawArguments& args) {
					ofPushMatrix();
					{
						auto scale = min(args.localBounds.width, args.localBounds.height);
						ofScale(scale, scale, 1.0f);

						// Draw background if locked
						if (this->parameters.locked) {
							ofPushStyle();
							{
								ofSetColor(200, 100, 100);
								ofFill();
								ofDrawRectangle(0, 0, 1, 1);
							}
							ofPopStyle();
						}

						// Draw curves
						Curve::DrawArguments curveDrawArgs;
						{
							curveDrawArgs.showInfo = this->parameters.debug.drawInfo;
						}
						
						// Draw the curves (and potentially annotations)
						for (const auto& curve : this->curves) {
							curve.draw(curveDrawArgs);
						}
						this->newCurve.draw(curveDrawArgs);

						// Draw outline
						ofPushStyle();
						{
							ofNoFill();
							ofDrawRectangle(0, 0, 1, 1);
						}
						ofPopStyle();
					}
					ofPopMatrix();
				};

				panel->onMouse += [this, panel](ofxCvGui::MouseArguments& args) {
					if (this->parameters.locked) {
						return;
					}

					auto scale = min(panel->getWidth(), panel->getHeight());

					glm::vec3 vertex(args.local.x / scale, args.local.y / scale, 0.0f);

					switch (args.action) {
					case ofxCvGui::MouseArguments::Action::Pressed:
					{
						if (args.button == 0) {
							this->newCurve.addVertex(vertex);
							args.takeMousePress(panel);
						}
						else if (args.button == 2) {
							this->clearDrawing();
						}
						break;
					}
					case ofxCvGui::MouseArguments::Action::Dragged:
					{
						this->newCurve.addVertex(vertex);
						break;
					}
					case ofxCvGui::MouseArguments::Action::Released:
					{
						this->curves.push_back(newCurve);
						this->newCurve.clear();
						break;
					}
					}
				};

				return panel;
			}

			//----------
			void
				Target::drawObject()
			{
				ofPushMatrix();
				{
					ofScale(this->parameters.scale.get());
					ofTranslate(-0.5, -0.5, 0.0f);

					ofPushStyle();
					{
						ofNoFill();
						ofDrawRectangle(0, 0, 1, 1);

					}
					ofPopStyle();

					// Draw curves
					Curve::DrawArguments curveDrawArgs;
					{
						curveDrawArgs.showInfo = this->parameters.debug.drawInfo;
					}
					for (const auto& curve : this->curves) {
						curve.draw(curveDrawArgs);
					}
					this->newCurve.draw(curveDrawArgs);
				}
				ofPopMatrix();
			}

			//----------
			void
				Target::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addTitle("Drawing");

				inspector->addLiveValue<size_t>("Curve count", [this]() {
					return this->curves.size();
					});

				inspector->addButton("Clear", [this]() {
					this->clearDrawing();
					});

				inspector->addButton("Normalise", [this]() {
					this->normaliseDrawing();
					});

				inspector->addButton("Simple line", [this]() {
					this->simpleLineDrawing();
					});

				inspector->addButton("Delete last", [this]() {
					if (!this->curves.empty()) {
						this->curves.resize(this->curves.size() - 1);
					}
					});

				inspector->addButton("Resample", [this]() {
					auto result = ofSystemTextBoxDialog("Resolution", "1024");
					auto resultNumber = ofToInt(result);
					if (resultNumber < 1) {
						return;
					}
					this->resampleDrawing(resultNumber);
					});
			}

			//----------
			void
				Target::serialize(nlohmann::json& json)
			{
				{
					nlohmann::json jsonCurves;
					for (const auto& curve : this->curves) {
						nlohmann::json jsonCurve;
						curve.serialize(jsonCurve);
						jsonCurves.push_back(jsonCurve);
					}
					json["curves"] = jsonCurves;
				}
			}

			//----------
			void
				Target::deserialize(const nlohmann::json& json)
			{
				this->curves.clear();

				if (json.contains("curves")) {
					const auto& jsonCurves = json["curves"];

					for (auto& jsonCurve : jsonCurves) {
						Curve curve;
						curve.deserialize(jsonCurve);
						this->curves.push_back(curve);
					}
				}
			}

			//----------
			void
				Target::clearDrawing()
			{
				this->curves.clear();
			}

			//----------
			void
				Target::normaliseDrawing()
			{
				// find the current bounds
				ofRectangle bounds(0.5, 0.5, 0, 0);
				for (const auto& curve : this->curves) {
					const auto & vertices = curve.getVertices();
					for (const auto& vertex : vertices) {
						bounds.growToInclude(vertex);
					}
				}

				// calculate changes
				auto scaleIn = max(bounds.width, bounds.height);
				auto scaleUpFactor = 1.0f / scaleIn;
				auto oldCenter = bounds.getCenter();

				for (auto& curve : this->curves) {
					auto & vertices = curve.getVerticesForWriting();
					for (auto& vertex : vertices) {
						vertex = (vertex - oldCenter) * scaleUpFactor + glm::vec3(0.5, 0.5, 0.0f);
					}
					curve.calc();
				}
			}

			//----------
			void
				Target::simpleLineDrawing()
			{
				this->curves.clear();
				this->curves.resize(1);
				auto& curve = this->curves.front();

				size_t resolution = 16;
				for (size_t i = 0; i < resolution; i++) {
					curve.addVertex({
						ofMap(i, 0, resolution - 1, 0, 1)
						, 0.5f
						, 0.0f
						});
				}
			}

			//----------
			vector<Target::Curve>
				Target::getCurves() const
			{
				return this->curves;
			}

			//----------
			vector<Target::Curve>
				Target::getResampledCurves(size_t size) const
			{
				if (this->curves.empty()) {
					return vector<Target::Curve>();
				}

				vector<Curve> newCurves;

				auto firstVertex = this->curves.begin();

				// Get total length
				float totalLength = 0.0f;
				for (const auto& curve : this->curves) {
					totalLength += curve.getLength();
				}

				// Resamples curves to give them appropriate resolution
				size_t totalSizeSoFar = 0;
				for (size_t i = 0; i < this->curves.size() - 1; i++) {
					auto curve = this->curves[i];
					auto curveSize = (size_t)(curve.getLength() / totalLength * (float)size);
					curve.resample(curveSize);
					newCurves.push_back(curve);
					totalSizeSoFar += curveSize;
				}

				// For the last one - just give it all remaining resolution
				// This way we avoid rounding errors
				{
					auto curve = this->curves.back();
					curve.resample(size - totalSizeSoFar);
					newCurves.push_back(curve);
				}

				return newCurves;
			}

			//----------
			void
				Target::resampleDrawing(size_t size)
			{
				this->curves = this->getResampledCurves(size);
			}

			//----------
			vector<glm::vec3>
				Target::getTargetPoints() const
			{
				return this->getTargetPointsForCurves(this->curves);
			}

			//----------
			vector<glm::vec3>
				Target::getTargetPointsForCurves(const vector<Curve>& curves) const
			{
				vector<glm::vec3> targetPoints;

				auto ourTransform = this->getTransform();

				for (const auto& curve : curves) {
					const auto& vertices = curve.getVertices();
					for (const auto& vertex : vertices) {
						auto vertexObject = (vertex - glm::vec3(0.5, 0.5, 0.0)) * this->parameters.scale.get();
						auto vertexWorld = ofxCeres::VectorMath::applyTransform(ourTransform, vertexObject);
						targetPoints.push_back(vertexWorld);
					}
				}

				return targetPoints;
			}
		}
	}
}
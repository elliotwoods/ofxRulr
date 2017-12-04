#include "pch_Plugin_ArUco.h"
#include "AlignMarkerMap.h"

#include "MarkerMap.h"

namespace ofxRulr {
	namespace Nodes {
		namespace ArUco {
#pragma mark Capture
			//----------
			AlignMarkerMap::Constraint::Constraint() {
				RULR_SERIALIZE_LISTENERS;
			}

			//----------
			std::string AlignMarkerMap::Constraint::getDisplayString() const {
				return "Constraint";
			}

			//----------
			ofxCvGui::ElementPtr AlignMarkerMap::Constraint::getDataDisplay() {
				auto element = ofxCvGui::makeElement();
				auto selectMarker = make_shared<ofxCvGui::Widgets::EditableValue<int>>(this->markerID);
				{
					element->addChild(selectMarker);
				}

				auto selectPlane = make_shared<ofxCvGui::Widgets::MultipleChoice>("Plane");
				{
					selectPlane->addOptions({ "X", "Y", "Z" });
					selectPlane->entangle(this->plane);
					element->addChild(selectPlane);
				}
				auto selectPoints = make_shared<ofxCvGui::Widgets::MultipleChoice>("Points");
				{
					selectPoints->addOptions({ "All", "Center" });
					selectPoints->entangle(this->points);
					element->addChild(selectPoints);
				}
				auto setOffset = make_shared<ofxCvGui::Widgets::EditableValue<float>>(this->offset);
				{
					element->addChild(setOffset);
				}

				element->onBoundsChange += [this, selectMarker, selectPlane, selectPoints, setOffset](ofxCvGui::BoundsChangeArguments & args) {
					auto bounds = args.localBounds;
					bounds.height = 40.0f;
					selectMarker->setBounds(bounds);
					bounds.height = 50.0f;
					bounds.y = 40.0f;
					selectPlane->setBounds(bounds);	
					bounds.y = 90.0f;
					selectPoints->setBounds(bounds);
					bounds.y = 130.0f;
					setOffset->setBounds(bounds);
				};

				element->onDraw += [this](ofxCvGui::DrawArguments) {
					if (this->isSelected()) {
						auto residual = sqrt(this->residual.get());
						if (residual != 0.0f) {
							ofxCvGui::Utils::drawText("Residual " + ofToString(residual, 3) + "m"
								, -10, 170, false);
						}
					}
				};

				element->setHeight(190.0f);

				return element;
			}

			//----------
			void AlignMarkerMap::Constraint::serialize(Json::Value & json) {
				json << this->markerID;
				json << this->plane;
				json << this->offset;
				json << this->points;
				json << this->residual;
			}

			//----------
			void AlignMarkerMap::Constraint::deserialize(const Json::Value & json) {
				json >> this->markerID;
				json >> this->plane;
				json >> this->offset;
				json >> this->points;
				json >> this->residual;
			}

			//----------
			float AlignMarkerMap::Constraint::getResidual(const ofMatrix4x4 & transform) {
				if (this->cachedPoints.empty()) {
					throw(ofxRulr::Exception("No points cached for residual on Constraint"));
				}

				float residual = 0.0f;
				for (const auto & point : this->cachedPoints) {
					auto transformedPoint = point * transform;
					const auto & delta = transformedPoint[this->plane.get()] - this->offset.get();
					residual += delta * delta;
				}
				return residual;
			}

			//----------
			void AlignMarkerMap::Constraint::updatePoints(shared_ptr<aruco::MarkerMap> markerMap) {
				//will throw cv::exception if not available
				const auto & markerInfo = markerMap->getMarker3DInfo(this->markerID);
				this->cachedPoints.clear();
				for (const auto & point : markerInfo.points) {
					this->cachedPoints.push_back(ofxCv::toOf(point));
				}

				//take average if we selected 'center'
				if (this->points.get() == (int) Points::Center) {
					ofVec3f accumulatePoint;
					for (const auto & point : this->cachedPoints) {
						accumulatePoint += point;
					}
					this->cachedPoints.clear();
					this->cachedPoints.push_back(accumulatePoint / (float) markerInfo.size());
				}

				this->residual = this->getResidual(ofMatrix4x4());
				{
					auto color = ofColor(100, 100, 100, 255);
					color[this->plane.get()] = 255;
					this->color = color;
				}
			}

			//----------
			void AlignMarkerMap::Constraint::drawWorld() {
				ofMesh lines;
				lines.setMode(ofPrimitiveMode::OF_PRIMITIVE_LINES);

				auto planeIndex = this->plane.get();

				for (const auto & point : this->cachedPoints) {
					lines.addVertex(point);
					{
						auto pointFlattened = point;
						pointFlattened[planeIndex] = this->offset.get();
						lines.addVertex(pointFlattened);
					}

					{
						lines.addColor(this->color.get());
						lines.addColor(ofFloatColor(0.5, 0.5, 0.5, 1.0));
					}
				}

				lines.draw();
			}

#pragma mark Model
			//----------
			unsigned int AlignMarkerMap::Model::getParameterCount() const {
				return 6;
			}

			//----------
			void AlignMarkerMap::Model::getResidual(shared_ptr<Constraint> constraint, double & residual, double * gradient) const {
				if (gradient) {
					throw(ofxRulr::Exception("Gradient not implemented for this model"));
				}
				residual = (double) constraint->getResidual(this->cachedModel);
			}

			//----------
			void AlignMarkerMap::Model::evaluate(shared_ptr<Constraint> &) const {
				//do nothing here
			}

			//----------
			void AlignMarkerMap::Model::cacheModel() {
				this->cachedModel = this->getTransform();
			}

			//----------
			ofMatrix4x4 AlignMarkerMap::Model::getTransform() const {
				return ofMatrix4x4::newRotationMatrix(this->parameters[0], ofVec3f(1, 0, 0)
					, this->parameters[1], ofVec3f(0, 1, 0)
					, this->parameters[2], ofVec3f(0, 0, 1))
					* ofMatrix4x4::newTranslationMatrix(this->parameters[3]
						, this->parameters[4]
						, this->parameters[5]);
			}

#pragma mark FitRoomPlanes
			//----------
			AlignMarkerMap::AlignMarkerMap() {
				RULR_NODE_INIT_LISTENER;
				this->setIcon(Nodes::GraphicsManager::X().getIcon("ArUco::Base"));
			}

			//----------
			std::string AlignMarkerMap::getTypeName() const {
				return "ArUco::AlignMarkerMap";
			}

			//----------
			void AlignMarkerMap::init() {
				RULR_NODE_UPDATE_LISTENER;
				RULR_NODE_SERIALIZATION_LISTENERS;
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_DRAW_WORLD_LISTENER;

				this->addInput<MarkerMap>();

				{
					auto panel = ofxCvGui::Panels::makeWidgets();
					panel->addTitle("Constraints", ofxCvGui::Widgets::Title::H2);
					this->constraints.populateWidgets(panel);
					this->panel = panel;
				}

				this->manageParameters(this->parameters);
			}

			//----------
			void AlignMarkerMap::update() {
				auto markerMap = this->getInput<MarkerMap>();
				if (markerMap) {
					auto markerMapRaw = markerMap->getMarkerMap();
					if (markerMapRaw) {
						auto constraints = this->constraints.getSelection();
						for (auto constraint : constraints) {
							try {
								constraint->updatePoints(markerMapRaw);
							}
							RULR_CATCH_ALL_TO_ERROR;
						}
					}
				}
			}

			//----------
			void AlignMarkerMap::drawWorldStage() {
				auto constraints = this->constraints.getSelection();
				for (auto constraint : constraints) {
					constraint->drawWorld();
				}
			}

			//----------
			ofxCvGui::PanelPtr AlignMarkerMap::getPanel() {
				return this->panel;
			}

			//----------
			void AlignMarkerMap::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Add...", [this]() {
					auto markerStringComposite = ofSystemTextBoxDialog("Marker index");
					auto markertIndexStrings = ofSplitString(markerStringComposite, ",");
					for (const auto & markerString : markertIndexStrings) {
						if (!markerString.empty()) {
							auto index = ofToInt(markerString);
							auto capture = make_shared<Constraint>();
							this->constraints.add(capture);
							capture->markerID = index;
						}
					}
				})->setHeight(100.0f);

				inspector->addButton("Fit", [this]() {
					try {
						ofxRulr::Utils::ScopedProcess scopedProcess("Align Marker Map");
						this->fit();
						scopedProcess.end();
					}
					RULR_CATCH_ALL_TO_ALERT;
				})->setHeight(100.0f);
			}

			//----------
			void AlignMarkerMap::serialize(Json::Value & json) {
				this->constraints.serialize(json["captureSet"]);
			}

			//----------
			void AlignMarkerMap::deserialize(const Json::Value & json) {
				this->constraints.deserialize(json["captureSet"]);
			}

			//----------
			void AlignMarkerMap::fit() {
				this->throwIfMissingAConnection<MarkerMap>();
				auto markerMap = this->getInput<MarkerMap>();
				auto markerMapRaw = markerMap->getMarkerMap();
				if (!markerMapRaw) {
					throw(ofxRulr::Exception("Marker map not allocated"));
				}

				auto constraints = this->constraints.getSelection();
				for (const auto & constraint : constraints) {
					constraint->updatePoints(markerMapRaw);
				}

				ofxNonLinearFit::Fit<Model> fit;
				auto model = Model();

				double residual;
				fit.optimise(model, &constraints, &residual);
				this->parameters.residual.set(residual);

				//transform the marker points
				const auto transform = model.getTransform();
				for (auto & marker : * markerMapRaw) {
					for (auto & point : marker.points) {
						point = ofxCv::toCv(ofxCv::toOf(point) * transform);
					}
				}
			}
		}
	}
}
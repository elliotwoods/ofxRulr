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
				auto showResidual = make_shared<ofxCvGui::Widgets::LiveValue<float>>(this->residual);

				element->onBoundsChange += [this, selectMarker, selectPlane, selectPoints, showResidual](ofxCvGui::BoundsChangeArguments & args) {
					auto bounds = args.localBounds;
					bounds.height = 40.0f;
					selectMarker->setBounds(bounds);
					bounds.height = 50.0f;
					bounds.y = 40.0f;
					selectPlane->setBounds(bounds);
					bounds.y = 90.0f;
					selectPoints->setBounds(bounds);
					bounds.y = 130.0f;
					showResidual->setBounds(bounds);
				};
				element->setHeight(170.0f);

				return element;
			}

			//----------
			void AlignMarkerMap::Constraint::serialize(Json::Value & json) {
				json << this->markerID;
				json << this->plane;
				json << this->points;
				json << this->residual;
			}

			//----------
			void AlignMarkerMap::Constraint::deserialize(const Json::Value & json) {
				json >> this->markerID;
				json >> this->plane;
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
					const auto & delta = transformedPoint[this->points.get()];
					residual += delta * delta;
				}
				return residual;
			}

			//----------
			void AlignMarkerMap::Constraint::cachePointsForFit(shared_ptr<aruco::MarkerMap> markerMap) {
				//will throw cv::exception if not available
				const auto & markerInfo = markerMap->getMarker3DInfo(this->markerID);
				this->cachedPoints.clear();
				for (const auto & point : markerInfo) {
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

			}

			//----------
			void AlignMarkerMap::drawWorldStage() {

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
					constraint->cachePointsForFit(markerMapRaw);
				}

				ofxNonLinearFit::Fit<Model> fit;
				auto model = Model();

				double residual;
				fit.optimise(model, &constraints, &residual);
				this->parameters.residual.set(residual);

				//transform the marker points
				const auto transform = model.getTransform();
				for (auto & marker : * markerMapRaw) {
					for (auto & point : marker) {
						point = ofxCv::toCv(ofxCv::toOf(point) * transform);
					}
				}
			}
		}
	}
}
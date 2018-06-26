#include "pch_Plugin_BrightnessAssignmentMap.h"
#include "HistogramWidget.h"

namespace ofxRulr {
	namespace Nodes {
		namespace BAM {
			//----------
			World::World() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			std::string World::getTypeName() const {
				return "BAM::World";
			}

			//----------
			void World::init() {
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Nodes::Base>("Scene");
				this->manageParameters(this->parameters);
			}

			//----------
			void World::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				auto histogramWidget = make_shared<HistogramWidget>();
				auto histogramWidgetWeak = weak_ptr<HistogramWidget>(histogramWidget);
				auto updateHistogram = [this, histogramWidgetWeak]() {
					auto histogramWidget = histogramWidgetWeak.lock();
					if (histogramWidget) {
						cv::Mat histogram;
						auto projectors = this->getProjectors();
						for (auto projector : projectors) {
							auto & bamPass = projector->getPass(Pass::Level::BrightnessAssignmentMap, true);
							histogram = bamPass.getHistogram(histogram);
						}
						histogramWidget->setData(histogram);
					}
				};
				//updateHistogram();
				inspector->add(histogramWidget);

				inspector->addButton("Update histogram", updateHistogram);

				inspector->addButton("Export all...", [this]() {
					try {
						auto result = ofSystemLoadDialog("Output path", true);
						if (result.bSuccess) {
							this->exportAll(result.filePath);
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
				})->setHeight(100.0f);
			}

			//----------
			void World::registerProjector(shared_ptr<Projector> projector) {
				this->projectors.emplace_back(projector);
			}

			//----------
			void World::unregisterProjector(shared_ptr<Projector> projector) {
				for (auto it = this->projectors.begin(); it != this->projectors.end();) {
					auto otherProjector = it->lock();
					if (!otherProjector || otherProjector == projector) {
						it = this->projectors.erase(it);
					}
					else {
						++it;
					}
				}
			}

			//----------
			vector<shared_ptr<Projector>> World::getProjectors() const {
				//return only active projectors

				vector<shared_ptr<Projector>> projectors;
				for (auto projectorWeak : this->projectors) {
					auto projector = projectorWeak.lock();
					if (projector) {
						projectors.emplace_back(projector);
					}
				}
				return projectors;
			}

			//----------
			const ofParameterGroup & World::getParameters() const {
				return this->parameters;
			}

			//----------
			void World::exportAll(const string & folderPath) const {
				auto bamProjectors = this->getProjectors();
				for (auto bamProjector : bamProjectors) {
					auto projectorViewNode = bamProjector->getInput<Item::Projector>();
					auto pathBase = folderPath + "/" + projectorViewNode->getName();
					cout << "Exporting to " << pathBase << endl;

					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::BrightnessAssignmentMap, pathBase + "-BrightnessAssignmentMap.exr");
					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::BrightnessAssignmentMap, pathBase + "-BrightnessAssignmentMap.png");
					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::AvailabilityProjection, pathBase + "-AvailabilityProjection.exr");
					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::AvailabilityProjection, pathBase + "-AvailabilityProjection.png");
					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::AccumulateAvailability, pathBase + "-AccumulateAvailability.exr");
					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::AccumulateAvailability, pathBase + "-AccumulateAvailability.png");
					bamProjector->exportPass(ofxRulr::Nodes::BAM::Pass::Color, pathBase + "-Color.png");
				}
			}
		}
	}
}
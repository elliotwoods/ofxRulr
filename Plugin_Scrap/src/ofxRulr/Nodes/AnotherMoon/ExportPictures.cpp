#include "pch_Plugin_Scrap.h"
#include "ExportPictures.h"

#include "DrawMoon.h"
#include "Lasers.h"
#include "Moon.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			ExportPictures::ExportPictures()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				ExportPictures::getTypeName() const
			{
				return "AnotherMoon::ExportPictures";
			}

			//----------
			void
				ExportPictures::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;
				this->addInput<DrawMoon>();
				this->manageParameters(this->parameters);
			}

			//----------
			void
				ExportPictures::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Export pictures...", [this]() {
					try {
						auto result = ofSystemLoadDialog("Select output folder", true);
						if (result.bSuccess) {
							this->exportPictures(std::filesystem::path(result.filePath));
						}
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			void
				ExportPictures::exportPictures(std::filesystem::path& outputFolder) const
			{
				this->throwIfMissingAConnection<DrawMoon>();
				auto drawMoonNode = this->getInput<DrawMoon>();

				drawMoonNode->throwIfMissingAnyConnection();
				auto lasersNode = drawMoonNode->getInput<Lasers>();
				auto moonNode = drawMoonNode->getInput<Moon>();

				auto selectedLasers = lasersNode->getLasersSelected();

				// Draw start
				{
					Utils::ScopedProcess scopedProcess("Start pictures");

					moonNode->setPosition(this->parameters.start.position.get());
					moonNode->setDiameter(this->parameters.start.diameter.get());
					drawMoonNode->drawLasers(true);

					for (auto laser : selectedLasers) {
						const auto laserPositionIndex = laser->parameters.positionIndex.get();
						const auto outputPath = outputFolder / ("picture_start_" + ofToString(laserPositionIndex) + ".json");
						laser->exportLastPicture(outputPath);
					}

					scopedProcess.end();
				}

				// Draw end
				{
					Utils::ScopedProcess scopedProcess("End pictures");

					moonNode->setPosition(this->parameters.end.position.get());
					moonNode->setDiameter(this->parameters.end.diameter.get());
					drawMoonNode->drawLasers(true);

					for (auto laser : selectedLasers) {
						const auto laserPositionIndex = laser->parameters.positionIndex.get();
						const auto outputPath = outputFolder / ("picture_end_" + ofToString(laserPositionIndex) + ".json");
						laser->exportLastPicture(outputPath);
					}

					scopedProcess.end();
				}
			}
		}
	}
}
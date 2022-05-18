#include "pch_Plugin_Scrap.h"
#include "SynthesiseCaptures.h"

#include "Calibrate.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			SynthesiseCaptures::SynthesiseCaptures()
			{
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string
				SynthesiseCaptures::getTypeName() const
			{
				return "AnotherMoon::SynthesiseCaptures";
			}


			//----------
			void
				SynthesiseCaptures::init()
			{
				RULR_NODE_INSPECTOR_LISTENER;

				this->addInput<Calibrate>();

				this->manageParameters(this->parameters);
			}

			//----------
			void
				SynthesiseCaptures::populateInspector(ofxCvGui::InspectArguments& args)
			{
				auto inspector = args.inspector;
				inspector->addButton("Synthesise", [this]() {
					try {
						this->synthesise();
					}
					RULR_CATCH_ALL_TO_ALERT;
					}, OF_KEY_RETURN)->setHeight(100.0f);
			}

			//----------
			void
				SynthesiseCaptures::synthesise()
			{
				this->throwIfMissingAnyConnection();
				auto calibrate = this->getInput<Calibrate>();

				calibrate->throwIfMissingAConnection<Lasers>();
				auto lasersNode = calibrate->getInput<Lasers>();

				calibrate->throwIfMissingAConnection<Item::Camera>();
				auto cameraNode = calibrate->getInput<Item::Camera>();
				auto cameraIntrinsics = Models::Intrinsics{
					cameraNode->getWidth()
					, cameraNode->getHeight()
					, cameraNode->getViewInObjectSpace().getProjectionMatrix()
				};

				auto lasers = lasersNode->getLasersSelected();
				const auto & resolution = this->parameters.resolution.get();

				auto cameraCaptures = calibrate->getCameraCaptures().getSelection();
				for (auto cameraCapture : cameraCaptures) {
					// Get the camera model
					Models::Camera cameraModel {
						glm::inverse(cameraCapture->cameraTransform->getTransform())
						, cameraIntrinsics
					};

					auto laserCaptures = cameraCapture->laserCaptures.getSelection();
					for (auto laserCapture : laserCaptures) {
						// Disable existing
						if (this->parameters.disableExistingCaptures) {
							laserCapture->beamCaptures.selectNone();
						}

						// Find the laser
						shared_ptr<Laser> laser;
						for (auto it : lasers) {
							if (it->parameters.communications.address.get() == laserCapture->laserAddress) {
								laser = it;
								break;
							}
						}
						if (!laser) {
							continue;
						}

						// Create laser model
						auto laserModel = laser->getModel();

						// Add new
						for (int i = 0; i < resolution; i++) {
							for (int j = 0; j < resolution; j++) {
								// Get projection point
								glm::vec2 projectionPoint{
									ofMap(i, 0, resolution - 1, -1, 1)
									, ofMap(j, 0, resolution - 1, -1, 1)
								};

								// Project it into a ray
								auto ray = laserModel.castRayWorldSpace(projectionPoint);

								// Get the line in camera
								auto line = cameraModel.worldToImage(ray);

								// Store as new BeamCapture
								auto beamCapture = make_shared<Calibrate::BeamCapture>();
								beamCapture->parentSelection = &laserCapture->ourSelection;
								beamCapture->projectionPoint = projectionPoint;
								beamCapture->line = line;
								beamCapture->isOffset = false;
								laserCapture->beamCaptures.add(beamCapture);
							}
						}
					}
				}
			}
		}
	}
}
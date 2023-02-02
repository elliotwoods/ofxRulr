#include "pch_Plugin_Scrap.h"
#include "SortFiles.h"

namespace ofxRulr {
	namespace Nodes {
		namespace AnotherMoon {
			//----------
			SortFiles::SortFiles()
			{
				RULR_NODE_INIT_LISTENER;
				this->setIconGlyph(u8"\uf163");
			}

			//----------
			string
				SortFiles::getTypeName() const
			{
				return "AnotherMoon::SortFiles";
			}

			//---------
			void
				SortFiles::init()
			{
				this->addInput<Calibrate>();

				this->manageParameters(this->parameters);
				this->onPopulateInspector += [this](ofxCvGui::InspectArguments& args) {
					auto inspector = args.inspector;
					inspector->addButton("Sort files", [this]() {
						try {
							ofxRulr::Utils::ScopedProcess scopedProcess("Sort files");
							this->sortFiles();
							scopedProcess.end();
						}
						RULR_CATCH_ALL_TO_ALERT;
						}, OF_KEY_RETURN)->setHeight(100.0f);
				};
			}

			//---------
			void
				SortFiles::sortFiles()
			{
				this->throwIfMissingAConnection<Calibrate>();
				auto calibrate = this->getInput<Calibrate>();

				const auto targetPathRoot = this->parameters.targetDirectory.get();

				// Check we have parameter for target
				if (targetPathRoot.empty()) {
					throw(ofxRulr::Exception("No target path"));
				}

				const auto onlySelected = this->parameters.selectionOnly.get();
				const auto moveOrCopy = this->parameters.moveOrCopy.get();
				const auto dryRun = this->parameters.dryRun.get();
				const auto verbose = this->parameters.verbose.get();
				const auto openFileFirst = this->parameters.openFileFirst.get();

				auto cameraCaptures = onlySelected
					? calibrate->getCameraCaptures().getSelection()
					: calibrate->getCameraCaptures().getAllCaptures();

				{
					Utils::ScopedProcess scopedProcessCameraCaptures("Camera captures", false, cameraCaptures.size());
					for (auto cameraCapture : cameraCaptures) {
						auto laserCaptures = onlySelected
							? cameraCapture->laserCaptures.getSelection()
							: cameraCapture->laserCaptures.getAllCaptures();
						Utils::ScopedProcess scopedProcessLaserCaptures("Camera " + cameraCapture->getTimeString(), false, laserCaptures.size());

						auto cleanTimeString = cameraCapture->getTimeString();
						ofStringReplace(cleanTimeString, ":", ".");
						cameraCapture->directory = targetPathRoot / ("Camera " + cameraCapture->getDateString() + " " + cleanTimeString);

						for (auto laserCapture : laserCaptures) {
							auto beamCaptures = onlySelected
								? laserCapture->beamCaptures.getSelection()
								: laserCapture->beamCaptures.getAllCaptures();

							laserCapture->directory = cameraCapture->directory / ("Laser " + ofToString(laserCapture->serialNumber));

							Utils::ScopedProcess scopedProcessBeamCaptures("Laser " + ofToString(laserCapture->serialNumber), false, beamCaptures.size());

							int beamCaptureIndex = 0;
							for (auto beamCapture : beamCaptures) {
								auto moveOrCopyAction = [&](Calibrate::ImagePath& imagePath
									, const int beamCaptureIndex
									, const bool on) {
										auto priorLocalPath = calibrate->getLocalCopyPath(imagePath);
										if (!priorLocalPath.has_extension()) {
											throw(ofxRulr::Exception("Local path : " + priorLocalPath.string() + " is invalid"));
										}

										auto postLocalPath = laserCapture->directory
											/ ("Beam " + ofToString(beamCaptureIndex) + (on ? " (on)" : " (off)")
												+ priorLocalPath.extension().string());

										if (postLocalPath == priorLocalPath) {
											// No work to do
											return;
										}

										// Check if target already exists
										if (filesystem::exists(postLocalPath)) {
											if (filesystem::exists(priorLocalPath)) {
												// Both prior and post exist - Check file size matches
												if (filesystem::file_size(priorLocalPath)
													== filesystem::file_size(postLocalPath)) {
													// the target file is already correct
													return;
												}
												else {
													// Delete the file
													if (verbose) {
														cout << "Deleting target file because size isn't right" << postLocalPath.filename().string() << std::endl;
													}

													if (!dryRun) {
														filesystem::remove(postLocalPath);
													}
												}
											}
											else {
												// Only target exists
												return;
											}
										}

										// Error if source doesn't exist (e.g. filename might have been renamed by importer)
										if (!filesystem::exists(priorLocalPath)) {
											bool fileFound = false;
											// Try adding " 1", " 2" etc to filename
											for (int i = 0; i < 10; i++) {
												auto altPath = priorLocalPath.parent_path() /
													(priorLocalPath.filename().string() + " " + ofToString(i) + priorLocalPath.extension().string());
												if (filesystem::exists(altPath)) {
													priorLocalPath = altPath;
													break;
												}
											}
											if (!fileFound) {
												throw(Exception("Prior file doesn't exist : " + priorLocalPath.filename().string()));
											}
										}

										// Try to open the file first
										if (openFileFirst) {
											cv::imread(priorLocalPath.string());
										}

										// Print action
										if (verbose) {
											cout << moveOrCopy.toString() << " " << priorLocalPath.string() << " to " << postLocalPath.string();
										}

										if (!dryRun) {
											// Ensure folders exist
											{
												const auto parentPath = postLocalPath.parent_path();
												auto createdDirectories = filesystem::create_directories(parentPath);
												if (verbose) {
													if (createdDirectories) {
														cout << "Created directories for " << parentPath.string() << std::endl;
													}
													else {
														cout << "Didn't create directories for " << parentPath.string() << std::endl;
													}
												}
											}

											// Perform the copy or move
											{
												try {


													switch (moveOrCopy.get()) {
													case MoveOrCopy::Copy:
														filesystem::copy(priorLocalPath
															, postLocalPath);
														break;
													case MoveOrCopy::Move:
														filesystem::rename(priorLocalPath
															, postLocalPath);
														break;
													default:
														break;
													}

													if (verbose) {
														cout << " [OK]";
													}
													imagePath.localCopy = postLocalPath;
												}
												catch (filesystem::filesystem_error& error) {
													throw(Exception("Move/Copy failed : " + std::string(error.what())));
												}
											}
										}
										cout << std::endl;
								};

								Utils::ScopedProcess scopedProcessBeamCapture(ofToString(beamCaptureIndex), false);

								try {
									moveOrCopyAction(beamCapture->onImage
										, beamCaptureIndex
										, true);
									moveOrCopyAction(beamCapture->offImage
										, beamCaptureIndex
										, false);
								}
								catch (const Exception& e) {
									if (this->parameters.onError.get() == OnError::DisableBeam) {
										// Just disable this beam capture
										beamCapture->setSelected(false);
										RULR_ERROR << e.what();
									}
									else {
										// Really just throw out
										throw(e);
									}
								}

								beamCaptureIndex++;
							}
						}


					}
				}

			}
		}
	}
}
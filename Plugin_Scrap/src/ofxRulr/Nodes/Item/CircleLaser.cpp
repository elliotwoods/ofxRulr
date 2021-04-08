#include "pch_Plugin_Scrap.h"
#include "CircleLaser.h"

namespace ofxRulr {
	namespace Nodes {
		namespace Item {
			//----------
			CircleLaser::CircleLaser() {
				RULR_NODE_INIT_LISTENER;
			}

			//----------
			string CircleLaser::getTypeName() const {
				return "Item::CircleLaser";
			}

			//----------
			void CircleLaser::init() {
				RULR_NODE_INSPECTOR_LISTENER;
				RULR_NODE_UPDATE_LISTENER;

				this->manageParameters(this->parameters);

				{
					auto panel = ofxCvGui::Panels::makeBlank();
					panel->onDraw += [this](ofxCvGui::DrawArguments & args) {
						ofPushView();
						{
							ofViewport(args.globalBounds);

							ofSetMatrixMode(ofMatrixMode::OF_MATRIX_PROJECTION);
							ofLoadIdentityMatrix();
							ofSetMatrixMode(ofMatrixMode::OF_MATRIX_MODELVIEW);
							ofLoadIdentityMatrix();

							this->previewLine.draw();
						}
						ofPopView();
					};
					this->panel = panel;
				}
				this->cachedParameters.address = "";
				this->cachedParameters.port = 0;
			}

			//----------
			void CircleLaser::update() {
				if (this->parameters.address.get() != this->cachedParameters.address.get()
					|| this->parameters.port.get() != this->cachedParameters.port.get()) {
					this->reconnect();
				}
			}

			//----------
			ofxCvGui::PanelPtr CircleLaser::getPanel() {
				return this->panel;
			}

			//----------
			void CircleLaser::populateInspector(ofxCvGui::InspectArguments & inspectArgs) {
				auto inspector = inspectArgs.inspector;

				inspector->addButton("Draw point", [this]() {
					stringstream pointText(ofSystemTextBoxDialog("Point coordinate"));
					glm::vec2 position;
					pointText >> position;
					this->drawPoint(position);
				});
			}

			//----------
			void CircleLaser::drawLine(const ofPolyline & line) {
				if (!this->oscSender) {
					throw(ofxRulr::Exception("Laser is not connected"));
				}

				ofxOscMessage message;
				message.setAddress("/line");
				message.addIntArg(line.getVertices().size());
				for (const auto & vertex : line.getVertices()) {
					message.addFloatArg(vertex.x);
					message.addFloatArg(vertex.y);
				}
				
				this->oscSender->sendMessage(message);

				this->previewLine = line;
			}

			//----------
			void CircleLaser::drawPoint(const glm::vec2 & point) {
				cout << "Drawing point " << point << endl;
				
				ofPolyline line;
				line.addVertex({
					point.x
					, point.y
					, 0.0f });


				this->drawLine(line);
			}

			//----------
			void CircleLaser::drawCircle(const glm::vec2 & center, float radius) {
				cout << "Drawing circle " << center << ", " << radius << endl;

				ofPolyline line;
				
				auto count = this->parameters.resolution.get();
				double thetaStep = TWO_PI / (double) count;
				double theta = 0.0;

				for (int i = 0; i < count + 1; i++) {
					line.addVertex(sin(theta) * radius + center.x
						, cos(theta) * radius + center.y);
					theta += thetaStep;
				}

				this->drawLine(line);
			}

			//----------
			void CircleLaser::clearDrawing() {
				cout << "Drawing clear" << endl;

				ofPolyline line;
				this->drawLine(line);
			}

			//----------
			void CircleLaser::reconnect() {
				this->oscSender.reset();

				{
					auto oscSender = make_unique<ofxOscSender>();
					oscSender->setup(this->parameters.address, this->parameters.port);
					this->cachedParameters.address = this->parameters.address;
					this->cachedParameters.port = this->parameters.port;
					this->oscSender = move(oscSender);
				}
			}
		}
	}
}
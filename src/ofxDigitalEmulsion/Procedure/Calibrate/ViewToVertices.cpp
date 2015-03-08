#include "ViewToVertices.h"

#include "../../Item/View.h"
#include "../../Device/VideoOutput.h"
#include "IReferenceVertices.h"

#include "ofxSpinCursor.h"

namespace ofxDigitalEmulsion {
	namespace Procedure {
		namespace Calibrate {
			//---------
			ViewToVertices::ViewToVertices() {
				OFXDIGITALEMULSION_NODE_INIT_LISTENER;
			}

			//---------
			string ViewToVertices::getTypeName() const {
				return "Procedure::Calibrate::ViewToVertices";
			}

			//---------
			void ViewToVertices::init() {
				OFXDIGITALEMULSION_NODE_UPDATE_LISTENER;
				OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
				OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;

				this->addInput<Item::View>();
				auto videoOutputPin = this->addInput<Device::VideoOutput>();
				this->addInput<IReferenceVertices>();

				videoOutputPin->onNewConnectionTyped += [this](shared_ptr<Device::VideoOutput> videoOutput) {
					videoOutput->onDrawOutput.addListener([this](ofRectangle & outputRectangle) {
						ofxSpinCursor::draw(ofVec2f(100, 100));
					}, this);
				};
				videoOutputPin->onDeleteConnectionTyped += [this](shared_ptr<Device::VideoOutput> videoOutput) {
					videoOutput->onDrawOutput.removeListeners(this);
				};
			}

			//---------
			ofxCvGui::PanelPtr ViewToVertices::getView() {
				return this->view;
			}

			//---------
			void ViewToVertices::update() {
				
			}

			//---------
			void ViewToVertices::serialize(Json::Value & json) {

			}

			//---------
			void ViewToVertices::deserialize(const Json::Value & json) {

			}

			//---------
			void ViewToVertices::calibrate() {

			}

			//---------
			void ViewToVertices::populateInspector(ofxCvGui::ElementGroupPtr inspector) {

			}
		}
	}
}
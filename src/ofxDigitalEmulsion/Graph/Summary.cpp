#include "Summary.h"

#include "ofxCvGui/Widgets/Toggle.h"
#include "ofxCvGui/Widgets/LiveValue.h"
#include "ofxCvGui/Widgets/EditableValue.h"
#include "ofxCvGui/Widgets/Title.h"

using namespace ofxCvGui;

namespace ofxDigitalEmulsion {
	namespace Graph {
		//----------
		Summary::Summary(const Utils::Set<Graph::Node> & world) :
		world(world) {
			OFXDIGITALEMULSION_NODE_INIT_LISTENER;
		}

		//----------
		void Summary::init() {
			OFXDIGITALEMULSION_NODE_INSPECTOR_LISTENER;
			OFXDIGITALEMULSION_NODE_SERIALIZATION_LISTENERS;
		
			this->view = MAKE(ofxCvGui::Panels::World);
			this->view->onDrawWorld += [this](ofCamera &) {
				if (this->showGrid) {
					this->drawGrid();
				}
				for (const auto node : this->world) {
					node->drawWorld();
				}
			};

			this->grid = &ofxAssets::image("ofxDigitalEmulsion::grid-10");
			this->light.setDirectional();

			this->showCursor.addListener(this, &Summary::callbackShowCursor);
			this->showCursor.set("Show Cursor", false);
			this->showGrid.set("Show Grid", true);
			this->roomMinimum.set("Room minimum", ofVec3f(-5.0f, -5.0f, -5.0f));
			this->roomMaximum.set("Room maxmimim", ofVec3f(+5.0f, 0.0f, 0.0f));
			this->view->setGridEnabled(false);
		}

		//----------
		string Summary::getTypeName() const {
			return "Summary";
		}

		//----------
		ofxCvGui::PanelPtr Summary::getView() {
			return this->view;
		}

		//----------
		void Summary::serialize(Json::Value & json) {
			Utils::Serializable::serialize(this->showCursor, json);
			Utils::Serializable::serialize(this->showGrid, json);
			Utils::Serializable::serialize(this->roomMinimum, json);
			Utils::Serializable::serialize(this->roomMaximum, json);

			auto & camera = this->view->getCamera();
			auto & cameraJson = json["Camera"];
			cameraJson["Transform"] << camera.getGlobalTransformMatrix();
		}

		//----------
		void Summary::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->showCursor, json);
			this->view->setCursorEnabled(this->showCursor);
			Utils::Serializable::deserialize(this->showGrid, json);
			Utils::Serializable::deserialize(this->roomMinimum, json);
			Utils::Serializable::deserialize(this->roomMaximum, json);

			if (json.isMember("Camera")) {
				auto & cameraJson = json["Camera"];
				auto & camera = this->view->getCamera();
				ofMatrix4x4 transform;
				cameraJson["Transform"] >> transform;
				camera.setTransformMatrix(transform);

				//nudge the camera
				camera.move(ofVec3f());
			}
		}

		//----------
		void Summary::populateInspector(ofxCvGui::ElementGroupPtr inspector) {
			inspector->add(Widgets::Title::make("Cursor", Widgets::Title::Level::H3));
			inspector->add(Widgets::Toggle::make(this->showCursor));
			inspector->add(Widgets::LiveValue<ofVec3f>::make("Position", [this]() {
				return this->view->getCamera().getCursorWorld();
			}));

			inspector->add(Widgets::Title::make("Grid", Widgets::Title::Level::H3));
			inspector->add(Widgets::Toggle::make(this->showGrid));
			inspector->add(Widgets::EditableValue<ofVec3f>::make(this->roomMinimum));
			inspector->add(Widgets::EditableValue<ofVec3f>::make(this->roomMaximum));
		}

		//----------
		void Summary::callbackShowCursor(bool & showCursor) {
			this->view->setCursorEnabled(showCursor);
		}

		//----------
		void Summary::drawGrid() {
			const auto & roomMinimum = this->roomMinimum.get();
			const auto & roomMaximum = this->roomMaximum.get();
			auto & camera = this->view->getCamera();

			//--
			//axes
			//--
			//
			ofPushStyle();
			ofSetLineWidth(2.0f);
			//
			//x
			ofSetColor(200, 100, 100);
			ofPushMatrix();
			ofTranslate(0, roomMaximum.y, roomMaximum.z);
			ofLine(roomMinimum.x, 0, roomMaximum.x, 0);
			ofPopMatrix();
			//
			//y
			ofSetColor(100, 200, 100);
			ofLine(0, roomMaximum.y, roomMaximum.z, 0, roomMinimum.y, roomMaximum.z);
			//
			//z
			ofSetColor(100, 200, 100);
			ofLine(0, roomMaximum.y, roomMaximum.z, 0, roomMaximum.y, roomMinimum.z);
			//
			ofPopStyle();
			//
			//--



			//--
			//cursor lines
			//--
			//
			ofPushStyle();
			const auto cursorPosition = camera.getCursorWorld();
			//
			ofSetColor(0);
			ofSetLineWidth(2.0f);
			//back wall
			ofLine(cursorPosition.x, roomMaximum.y, roomMaximum.z, cursorPosition.x, roomMinimum.y, roomMaximum.z); //x
			ofLine(roomMinimum.x, cursorPosition.y, roomMaximum.z, roomMaximum.x, cursorPosition.y, roomMaximum.z); //y
			//
			//floor
			ofLine(cursorPosition.x, roomMaximum.y, roomMinimum.z, cursorPosition.x, roomMaximum.y, roomMaximum.z); //x
			ofLine(roomMinimum.x, roomMaximum.y, cursorPosition.z, roomMaximum.x, roomMaximum.y, cursorPosition.z); //z
			//
			ofPopStyle();
			//
			//--



			//--
			//grid
			//--
			//
			glEnable(GL_CULL_FACE);
			this->light.setOrientation(camera.getOrientationQuat());
			this->light.enable();
			ofPushMatrix();
			//
			//back wall
			ofPushMatrix();
			ofTranslate(0, 0, roomMaximum.z);
			for (int x = roomMinimum.x; x < roomMaximum.x; x++) {
				for (int y = roomMinimum.y; y < roomMaximum.y; y++) {
					this->grid->draw((float)x, (float)y, 1.0f, 1.0f);
				}
			}
			ofPopMatrix();
			//
			//floor
			ofTranslate(0, roomMaximum.y, 0.0f); // y = 0 is the floor
			//also translate so rotation is around start of room
			ofRotate(90, -1, 0, 0);
			ofTranslate(0, -roomMinimum.z, 0); // offset drawing to begin earlier
			for (int x = roomMinimum.x; x < roomMaximum.x; x++) {
				for (int z = roomMinimum.z; z < roomMaximum.z; z++) {
					this->grid->draw((float) x, (float)z, +1.0f, 1.0f);
				}
			}
			//
			ofPopMatrix();
			this->light.disable();
			ofDisableLighting();
			glDisable(GL_CULL_FACE);
			//
			//--

		}
	}
}
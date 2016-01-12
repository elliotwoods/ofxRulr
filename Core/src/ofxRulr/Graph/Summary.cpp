#include "pch_RulrCore.h"
#include "Summary.h"

using namespace ofxCvGui;

namespace ofxRulr {
	namespace Graph {
		//----------
		Summary::Summary() :
		world() {
			RULR_NODE_INIT_LISTENER;
			this->world = nullptr;
		}

		//----------
		string Summary::getTypeName() const {
			return "Summary";
		}

		//----------
		void Summary::init() {
			RULR_NODE_UPDATE_LISTENER;
			RULR_NODE_INSPECTOR_LISTENER;
			RULR_NODE_SERIALIZATION_LISTENERS;

			this->view = MAKE(ofxCvGui::Panels::World);
			this->view->onDrawWorld += [this](ofCamera &) {
				if (this->showGrid) {
					this->drawGrid();
				}
				if (this->world) {
					auto & world = *this->world;
					for (const auto node : world) {
						node->drawWorld();
					}
				}

			};

			this->grid = &ofxAssets::image("ofxRulr::grid-10");

#ifdef OFXCVGUI_USE_OFXGRABCAM
			this->showCursor.addListener(this, &Summary::callbackShowCursor);
#endif
			this->showCursor.set("Show Cursor", false);
			this->showGrid.set("Show Grid", true);
			this->roomMinimum.set("Room minimum", ofVec3f(-5.0f, -4.0f, 0.0f));
			this->roomMaximum.set("Room maxmimim", ofVec3f(+5.0f, 0.0f, 6.0f));
			this->view->setGridEnabled(false);
		}

		//----------
		void Summary::setWorld(const Utils::Set<Nodes::Base> & world) {
			this->world = &world;
		}

		//----------
		void Summary::update() {
			auto & camera = view->getCamera();
			this->light.setPosition(camera.getPosition());
			this->light.lookAt(camera.getCursorWorld());
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
			cameraJson["position"] << camera.getPosition();
			cameraJson["orientation"] << camera.getOrientationQuat().asVec4(); //cast as ofVec4f since ofQuaternion doesn't have serialisation
		}

		//----------
		void Summary::deserialize(const Json::Value & json) {
			Utils::Serializable::deserialize(this->showCursor, json);
			Utils::Serializable::deserialize(this->showGrid, json);
			Utils::Serializable::deserialize(this->roomMinimum, json);
			Utils::Serializable::deserialize(this->roomMaximum, json);

			auto & camera = this->view->getCamera();
			if (json.isMember("Camera")) {
				auto & cameraJson = json["Camera"];
				ofVec3f position;
				cameraJson["position"] >> position;
				camera.setPosition(position);

				ofQuaternion orientation;
				cameraJson["orientation"] >> (ofVec4f&) orientation;
				camera.setOrientation(orientation);
			}
			else {
				camera.setPosition(this->roomMinimum.get() * ofVec3f(0.0f, 1.0f, 1.0f));
				camera.lookAt(this->roomMaximum.get() * ofVec3f(0.0f, 1.0f, 1.0f), ofVec3f(0, -1, 0));
				camera.move(ofVec3f()); // nudge camera to update
			}
		}

		//----------
		void Summary::populateInspector(ofxCvGui::InspectArguments & inspectArguments) {
			auto inspector = inspectArguments.inspector;
			
#ifdef OFXCVGUI_USE_OFXGRABCAM
			inspector->add(Widgets::Title::make("Cursor", Widgets::Title::Level::H3));
			inspector->add(Widgets::Toggle::make(this->showCursor));

			inspector->add(Widgets::LiveValue<ofVec3f>::make("Position", [this]() {
				return this->view->getCamera().getCursorWorld();
			}));
#endif

			inspector->add(Widgets::Title::make("Grid", Widgets::Title::Level::H3));
			inspector->add(Widgets::Toggle::make(this->showGrid));
			inspector->add(Widgets::EditableValue<ofVec3f>::make(this->roomMinimum));
			inspector->add(Widgets::EditableValue<ofVec3f>::make(this->roomMaximum));
		}


#ifdef OFXCVGUI_USE_OFXGRABCAM
		//----------
		void Summary::callbackShowCursor(bool & showCursor) {
			this->view->setCursorEnabled(showCursor);
		}
#endif

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
			ofDrawLine(roomMinimum.x, 0, roomMaximum.x, 0);
			ofPopMatrix();
			//
			//y
			ofSetColor(100, 200, 100);
			ofDrawLine(0, roomMaximum.y, roomMaximum.z, 0, roomMinimum.y, roomMaximum.z);
			//
			//z
			ofSetColor(100, 200, 100);
			ofDrawLine(0, roomMaximum.y, roomMaximum.z, 0, roomMaximum.y, roomMinimum.z);
			//
			ofPopStyle();
			//
			//--



#ifdef OFXCVGUI_USE_OFXGRABCAM
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
			ofDrawLine(cursorPosition.x, roomMaximum.y, roomMaximum.z, cursorPosition.x, roomMinimum.y, roomMaximum.z); //x
			ofDrawLine(roomMinimum.x, cursorPosition.y, roomMaximum.z, roomMaximum.x, cursorPosition.y, roomMaximum.z); //y
			//
			//floor
			ofDrawLine(cursorPosition.x, roomMaximum.y, roomMinimum.z, cursorPosition.x, roomMaximum.y, roomMaximum.z); //x
			ofDrawLine(roomMinimum.x, roomMaximum.y, cursorPosition.z, roomMaximum.x, roomMaximum.y, cursorPosition.z); //z
			//
			ofPopStyle();
			//
			//--
#endif



			//--
			//grid
			//--
			//
			glEnable(GL_CULL_FACE);
			ofPushMatrix();
			//
			//back wall
			ofPushMatrix();
			{
				this->light.enable();
				ofTranslate(0, 0, roomMaximum.z);
				for (int x = roomMinimum.x; x < roomMaximum.x; x++) {
					for (int y = roomMinimum.y; y < roomMaximum.y; y++) {
						this->grid->draw((float)x, (float)y, 1.0f, 1.0f);
					}
				}
				this->light.disable();
				ofDisableLighting();
			}
			ofPopMatrix();
			//
			//floor
			ofTranslate(0, roomMaximum.y, roomMaximum.z); // y = 0 is the floor
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
			glDisable(GL_CULL_FACE);
			//
			//--

		}
	}
}